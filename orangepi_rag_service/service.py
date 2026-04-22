from __future__ import annotations

import time
import uuid
from threading import RLock

from orangepi_rag_service.composer.prompt import compose_system_prompt
from orangepi_rag_service.config.settings import Settings
from orangepi_rag_service.importer.catalog import load_catalog, write_catalog
from orangepi_rag_service.importer.legacy_text import parse_legacy_text
from orangepi_rag_service.models.memory import MemoryHit, MemoryRecord, utc_now_iso
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
        with self._lock:
            self.store.upsert_many(records)
            self._refresh_indices_locked()
        return len(records)

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
