from __future__ import annotations

import hashlib
import time
import uuid
from threading import RLock

from orangepi_rag_service.composer.prompt import compose_system_prompt
from orangepi_rag_service.config.settings import Settings
from orangepi_rag_service.importer.catalog import load_catalog, write_catalog
from orangepi_rag_service.importer.legacy_text import parse_legacy_text
from orangepi_rag_service.models.memory import ALLOWED_CATEGORIES, MemoryHit, MemoryRecord, utc_now_iso
from orangepi_rag_service.retrieval.engine import RetrievalEngine
from orangepi_rag_service.retrieval.intent import classify_intent, should_use_memory
from orangepi_rag_service.store.sqlite_store import SQLiteMemoryStore
from orangepi_rag_service.store.vector_index import EmbeddingIndex


class MemoryOrchestrator:
    def __init__(self, settings: Settings) -> None:
        self.settings = settings
        self.settings.ensure_directories()
        self.store = SQLiteMemoryStore(settings.db_path)
        self.vector_index = EmbeddingIndex(
            settings.vector_index_path,
            settings.embedding_model,
            backend_mode=settings.embedding_backend,
        )
        self.retrieval = RetrievalEngine(self.store, self.vector_index)
        self._lock = RLock()

    def bootstrap(self) -> None:
        self.store.initialize()
        if self.settings.catalog_path.exists():
            self.import_catalog(self.settings.catalog_path)
            return
        if self.settings.legacy_memory_path.exists():
            self.import_legacy_text(self.settings.legacy_memory_path, persist_catalog=True)
            return
        self._refresh_indices_locked()

    def health(self) -> dict:
        count = self.store.count_memories()
        return {
            "status": "ok",
            "index_version": self.store.get_meta("index_version", "uninitialized"),
            "memory_count": count,
            "last_rebuild_time": self.store.get_meta("last_rebuild_time", "uninitialized"),
            "embedding_backend": self.vector_index.backend_name,
            "degraded": self.vector_index.degraded,
        }

    def import_catalog(self, path, /) -> int:
        records = load_catalog(path)
        with self._lock:
            self.store.replace_all(records)
            self._refresh_indices_locked()
        return len(records)

    def import_legacy_text(self, path, /, persist_catalog: bool = True) -> int:
        records = parse_legacy_text(path)
        with self._lock:
            if persist_catalog:
                write_catalog(self.settings.catalog_path, records)
            self.store.replace_all(records)
            self._refresh_indices_locked()
        return len(records)

    def upsert_memories(self, records: list[MemoryRecord]) -> int:
        records = self._prepare_records_for_upsert(records)
        mqtt_notes = [record for record in records if record.category == "note" and record.source in {"mqtt", "mqtt_app"}]
        with self._lock:
            self.store.delete_note_duplicates(mqtt_notes)
            self.store.upsert_many(records)
            self._refresh_indices_locked()
        return len(records)

    def list_recent_memories(self, category: str | None = "note", limit: int = 20) -> list[dict]:
        if category is not None and category not in ALLOWED_CATEGORIES:
            raise ValueError(f"不支持的记忆分类: {category}")
        if limit < 1 or limit > 100:
            raise ValueError("limit 必须在 1 到 100 之间")
        records = self.store.fetch_recent_memories(category=category, limit=limit)
        return [record.as_dict() for record in records]

    def list_memories(
        self,
        category: str | None = None,
        query: str | None = None,
        active_state: str = "active",
        limit: int = 100,
    ) -> list[dict]:
        if category is not None and category not in ALLOWED_CATEGORIES:
            raise ValueError(f"不支持的记忆分类: {category}")
        if active_state not in {"active", "inactive", "all"}:
            raise ValueError("active 只支持 active、inactive 或 all")
        if limit < 1 or limit > 200:
            raise ValueError("limit 必须在 1 到 200 之间")
        records = self.store.list_memories(
            category=category,
            query_text=query.strip() if query else None,
            active_state=active_state,
            limit=limit,
        )
        return [record.as_dict() for record in records]

    def compose(self, query: str, session_mode: str, max_prompt_chars: int) -> dict:
        if session_mode != self.settings.default_session_mode:
            raise ValueError(f"不支持的 session_mode: {session_mode}")

        start_time = time.perf_counter()
        trace_id = uuid.uuid4().hex[:16]
        decision = classify_intent(query)
        hits = []
        if should_use_memory(query, decision):
            hits = self.retrieval.search(query, decision, limit=self.settings.max_hits)
        system_prompt, has_memory = compose_system_prompt(
            query=query,
            intent=decision.intent,
            hits=hits,
            max_prompt_chars=min(max_prompt_chars, self.settings.max_prompt_chars),
            max_summary_chars=self.settings.max_summary_chars,
        )
        elapsed_ms = int((time.perf_counter() - start_time) * 1000)
        return {
            "system_prompt": system_prompt,
            "intent": decision.intent,
            "hits": [self._hit_to_payload(hit) for hit in hits],
            "trace_id": trace_id,
            "elapsed_ms": elapsed_ms,
            "degraded": self.vector_index.degraded,
            "has_memory": has_memory,
        }

    def _refresh_indices_locked(self) -> None:
        records = self.store.fetch_active_memories()
        self.vector_index.rebuild(records)
        stamp = utc_now_iso()
        version = stamp.replace(":", "").replace("-", "")
        self.store.set_meta("last_rebuild_time", stamp)
        self.store.set_meta("index_version", version)

    @staticmethod
    def _prepare_records_for_upsert(records: list[MemoryRecord]) -> list[MemoryRecord]:
        prepared: dict[str, MemoryRecord] = {}
        for record in records:
            if record.category == "note" and record.source in {"mqtt", "mqtt_app"}:
                digest = hashlib.sha1(
                    f"{record.category}|{record.title}|{record.content.strip()}".encode("utf-8")
                ).hexdigest()[:16]
                record.id = f"note_{digest}"
            prepared[record.id] = record
        return list(prepared.values())

    @staticmethod
    def _hit_to_payload(hit: MemoryHit) -> dict:
        summary = hit.record.content.strip().replace("\n", " / ")
        if len(summary) > 120:
            summary = summary[:119].rstrip() + "…"
        return {
            "id": hit.record.id,
            "category": hit.record.category,
            "title": hit.record.title,
            "summary": summary,
            "score": round(float(hit.score), 4),
            "source": hit.source,
        }
