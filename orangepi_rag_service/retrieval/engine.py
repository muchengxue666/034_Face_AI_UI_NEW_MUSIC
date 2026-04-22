from __future__ import annotations

from collections import defaultdict

from orangepi_rag_service.models.memory import MemoryHit, MemoryRecord
from orangepi_rag_service.retrieval.intent import IntentDecision
from orangepi_rag_service.retrieval.tokenizer import build_fts_query, extract_terms, normalize_text
from orangepi_rag_service.store.sqlite_store import SQLiteMemoryStore
from orangepi_rag_service.store.vector_index import EmbeddingIndex


_RULE_FIRST_INTENTS = {"medication", "routine", "contact", "safety"}


class RetrievalEngine:
    def __init__(self, store: SQLiteMemoryStore, vector_index: EmbeddingIndex) -> None:
        self._store = store
        self._vector_index = vector_index

    def search(self, query: str, decision: IntentDecision, limit: int = 4) -> list[MemoryHit]:
        if decision.intent in _RULE_FIRST_INTENTS:
            hits = self._rule_first_search(query, decision.categories, limit)
            if hits:
                return hits
        return self._hybrid_search(query, decision.categories, limit)

    def _rule_first_search(self, query: str, categories: tuple[str, ...], limit: int) -> list[MemoryHit]:
        normalized = normalize_text(query)
        records = self._store.fetch_active_memories(categories)
        scored: list[MemoryHit] = []
        for record in records:
            score = 0.0
            haystack = "\n".join([record.title, record.content, " ".join(record.keywords)])
            for keyword in record.keywords:
                if keyword and keyword.lower() in normalized:
                    score += 3.0
            if record.title.lower() in normalized:
                score += 2.0
            if any(token in haystack.lower() for token in normalized.split()):
                score += 1.0
            if score > 0:
                score += record.priority / 100.0
                scored.append(MemoryHit(record=record, score=score, source="rule"))
        scored.sort(key=lambda hit: hit.score, reverse=True)
        if scored:
            return scored[:limit]

        lexical_hits = self._store.search_fts(build_fts_query(query), categories, limit)
        return [
            MemoryHit(record=record, score=max(0.1, 10.0 - rank), source="fts")
            for record, rank in lexical_hits[:limit]
        ]

    def _hybrid_search(self, query: str, categories: tuple[str, ...], limit: int) -> list[MemoryHit]:
        lexical_hits = self._store.search_fts(build_fts_query(query), categories, limit=limit * 2)
        lexical_by_id = {record.id: (record, rank) for record, rank in lexical_hits}

        vector_hits = self._vector_index.search(query, limit=limit * 2)
        records_by_id = {record.id: record for record in self._store.fetch_active_memories(categories)}
        query_terms = set(extract_terms(query))

        rrf_scores: defaultdict[str, float] = defaultdict(float)
        for rank, (record, _) in enumerate(lexical_hits, start=1):
            rrf_scores[record.id] += 1.0 / (60 + rank)

        for rank, vector_hit in enumerate(vector_hits, start=1):
            record = records_by_id.get(vector_hit.memory_id)
            if not record:
                continue
            if not lexical_hits:
                record_terms = set(extract_terms(" ".join([record.title, record.content, " ".join(record.keywords)])))
                overlap = len(query_terms & record_terms)
                if overlap == 0 and vector_hit.score < 0.35:
                    continue
            rrf_scores[vector_hit.memory_id] += 1.0 / (60 + rank)

        ordered_ids = sorted(rrf_scores, key=rrf_scores.get, reverse=True)[:limit]
        results: list[MemoryHit] = []
        for memory_id in ordered_ids:
            record = records_by_id.get(memory_id)
            if not record and memory_id in lexical_by_id:
                record = lexical_by_id[memory_id][0]
            if not record:
                continue
            source = "hybrid"
            results.append(MemoryHit(record=record, score=rrf_scores[memory_id], source=source))
        return results
