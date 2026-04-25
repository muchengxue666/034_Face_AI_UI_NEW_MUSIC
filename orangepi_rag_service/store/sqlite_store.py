from __future__ import annotations

import json
import sqlite3
from datetime import datetime, timezone
from pathlib import Path

from orangepi_rag_service.models.memory import MemoryRecord
from orangepi_rag_service.retrieval.tokenizer import build_fts_document


class SQLiteMemoryStore:
    def __init__(self, db_path: Path) -> None:
        self._db_path = db_path
        self._db_path.parent.mkdir(parents=True, exist_ok=True)

    def _connect(self) -> sqlite3.Connection:
        conn = sqlite3.connect(self._db_path)
        conn.row_factory = sqlite3.Row
        return conn

    def initialize(self) -> None:
        conn = self._connect()
        try:
            conn.executescript(
                """
                CREATE TABLE IF NOT EXISTS memories (
                    id TEXT PRIMARY KEY,
                    category TEXT NOT NULL,
                    title TEXT NOT NULL,
                    content TEXT NOT NULL,
                    keywords TEXT NOT NULL,
                    source TEXT NOT NULL,
                    priority INTEGER NOT NULL DEFAULT 50,
                    effective_at TEXT,
                    expires_at TEXT,
                    is_active INTEGER NOT NULL DEFAULT 1,
                    updated_at TEXT NOT NULL
                );

                CREATE VIRTUAL TABLE IF NOT EXISTS memories_fts USING fts5(
                    memory_id UNINDEXED,
                    tokenized,
                    tokenize='unicode61'
                );

                CREATE TABLE IF NOT EXISTS service_meta (
                    key TEXT PRIMARY KEY,
                    value TEXT NOT NULL
                );
                """
            )
            conn.commit()
        finally:
            conn.close()

    def replace_all(self, records: list[MemoryRecord]) -> None:
        conn = self._connect()
        try:
            conn.execute("DELETE FROM memories_fts")
            conn.execute("DELETE FROM memories")
            self._upsert_many(conn, records)
            conn.commit()
        finally:
            conn.close()

    def upsert_many(self, records: list[MemoryRecord]) -> None:
        conn = self._connect()
        try:
            self._upsert_many(conn, records)
            conn.commit()
        finally:
            conn.close()

    def delete_note_duplicates(self, records: list[MemoryRecord]) -> int:
        if not records:
            return 0

        deleted = 0
        conn = self._connect()
        try:
            for record in records:
                rows = conn.execute(
                    """
                    SELECT id FROM memories
                    WHERE category = 'note'
                      AND title = ?
                      AND content = ?
                      AND id <> ?
                      AND source IN ('mqtt', 'mqtt_app')
                    """,
                    (record.title, record.content, record.id),
                ).fetchall()
                ids = [row["id"] for row in rows]
                if not ids:
                    continue
                placeholders = ", ".join("?" for _ in ids)
                conn.execute(f"DELETE FROM memories_fts WHERE memory_id IN ({placeholders})", ids)
                conn.execute(f"DELETE FROM memories WHERE id IN ({placeholders})", ids)
                deleted += len(ids)
            conn.commit()
        finally:
            conn.close()
        return deleted

    def _upsert_many(self, conn: sqlite3.Connection, records: list[MemoryRecord]) -> None:
        for record in records:
            conn.execute(
                """
                INSERT INTO memories (
                    id, category, title, content, keywords, source, priority,
                    effective_at, expires_at, is_active, updated_at
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                ON CONFLICT(id) DO UPDATE SET
                    category=excluded.category,
                    title=excluded.title,
                    content=excluded.content,
                    keywords=excluded.keywords,
                    source=excluded.source,
                    priority=excluded.priority,
                    effective_at=excluded.effective_at,
                    expires_at=excluded.expires_at,
                    is_active=excluded.is_active,
                    updated_at=excluded.updated_at
                """,
                (
                    record.id,
                    record.category,
                    record.title,
                    record.content,
                    json.dumps(record.keywords, ensure_ascii=False),
                    record.source,
                    record.priority,
                    record.effective_at,
                    record.expires_at,
                    1 if record.is_active else 0,
                    record.updated_at,
                ),
            )
            conn.execute("DELETE FROM memories_fts WHERE memory_id = ?", (record.id,))
            conn.execute(
                "INSERT INTO memories_fts (memory_id, tokenized) VALUES (?, ?)",
                (
                    record.id,
                    build_fts_document(
                        record.category,
                        record.title,
                        record.content,
                        " ".join(record.keywords),
                    ),
                ),
            )

    def _active_clause(self) -> tuple[str, tuple[str, str]]:
        now = datetime.now(timezone.utc).replace(microsecond=0).isoformat()
        return (
            """
            m.is_active = 1
            AND (m.effective_at IS NULL OR m.effective_at <= ?)
            AND (m.expires_at IS NULL OR m.expires_at >= ?)
            """,
            (now, now),
        )

    def fetch_active_memories(self, categories: tuple[str, ...] | None = None) -> list[MemoryRecord]:
        clause, params = self._active_clause()
        query = f"SELECT * FROM memories m WHERE {clause}"
        values: list[object] = list(params)
        if categories:
            placeholders = ", ".join("?" for _ in categories)
            query += f" AND m.category IN ({placeholders})"
            values.extend(categories)
        query += " ORDER BY m.priority DESC, m.updated_at DESC"

        conn = self._connect()
        try:
            rows = conn.execute(query, values).fetchall()
        finally:
            conn.close()
        return [self._row_to_record(row) for row in rows]

    def list_memories(
        self,
        category: str | None = None,
        query_text: str | None = None,
        active_state: str = "active",
        limit: int = 100,
    ) -> list[MemoryRecord]:
        query = "SELECT * FROM memories m WHERE 1 = 1"
        values: list[object] = []

        if category:
            query += " AND m.category = ?"
            values.append(category)
        if active_state == "active":
            query += " AND m.is_active = 1"
        elif active_state == "inactive":
            query += " AND m.is_active = 0"

        if query_text:
            like_text = f"%{query_text}%"
            query += """
                AND (
                    m.id LIKE ?
                    OR m.title LIKE ?
                    OR m.content LIKE ?
                    OR m.keywords LIKE ?
                    OR m.source LIKE ?
                )
            """
            values.extend([like_text, like_text, like_text, like_text, like_text])

        query += " ORDER BY m.updated_at DESC, m.priority DESC LIMIT ?"
        values.append(limit)

        conn = self._connect()
        try:
            rows = conn.execute(query, values).fetchall()
        finally:
            conn.close()
        return [self._row_to_record(row) for row in rows]

    def fetch_recent_memories(self, category: str | None = "note", limit: int = 20) -> list[MemoryRecord]:
        clause, params = self._active_clause()
        query = f"SELECT * FROM memories m WHERE {clause}"
        values: list[object] = list(params)
        if category:
            query += " AND m.category = ?"
            values.append(category)
        query += " ORDER BY m.updated_at DESC, m.priority DESC LIMIT ?"
        values.append(limit)

        conn = self._connect()
        try:
            rows = conn.execute(query, values).fetchall()
        finally:
            conn.close()
        return [self._row_to_record(row) for row in rows]

    def search_fts(self, fts_query: str, categories: tuple[str, ...] | None = None, limit: int = 8) -> list[tuple[MemoryRecord, float]]:
        if not fts_query:
            return []

        clause, params = self._active_clause()
        category_sql = ""
        values: list[object] = [fts_query, *params]
        if categories:
            placeholders = ", ".join("?" for _ in categories)
            category_sql = f" AND m.category IN ({placeholders})"
            values.extend(categories)
        values.append(limit)

        query = f"""
            SELECT m.*, bm25(memories_fts) AS rank
            FROM memories_fts
            JOIN memories m ON m.id = memories_fts.memory_id
            WHERE memories_fts MATCH ?
              AND {clause}
              {category_sql}
            ORDER BY rank ASC, m.priority DESC
            LIMIT ?
        """

        conn = self._connect()
        try:
            rows = conn.execute(query, values).fetchall()
        finally:
            conn.close()

        results: list[tuple[MemoryRecord, float]] = []
        for row in rows:
            rank = float(row["rank"]) if row["rank"] is not None else 0.0
            results.append((self._row_to_record(row), rank))
        return results

    def count_memories(self) -> int:
        conn = self._connect()
        try:
            row = conn.execute("SELECT COUNT(*) AS total FROM memories").fetchone()
        finally:
            conn.close()
        return int(row["total"]) if row else 0

    def set_meta(self, key: str, value: str) -> None:
        conn = self._connect()
        try:
            conn.execute(
                """
                INSERT INTO service_meta (key, value) VALUES (?, ?)
                ON CONFLICT(key) DO UPDATE SET value=excluded.value
                """,
                (key, value),
            )
            conn.commit()
        finally:
            conn.close()

    def get_meta(self, key: str, default: str = "") -> str:
        conn = self._connect()
        try:
            row = conn.execute("SELECT value FROM service_meta WHERE key = ?", (key,)).fetchone()
        finally:
            conn.close()
        return row["value"] if row else default

    @staticmethod
    def _row_to_record(row: sqlite3.Row) -> MemoryRecord:
        return MemoryRecord(
            id=row["id"],
            category=row["category"],
            title=row["title"],
            content=row["content"],
            keywords=json.loads(row["keywords"]),
            source=row["source"],
            priority=int(row["priority"]),
            effective_at=row["effective_at"],
            expires_at=row["expires_at"],
            is_active=bool(row["is_active"]),
            updated_at=row["updated_at"],
        )
