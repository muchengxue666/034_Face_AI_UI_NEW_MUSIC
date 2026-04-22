from __future__ import annotations

import hashlib
from dataclasses import dataclass
from pathlib import Path

import numpy as np

from orangepi_rag_service.models.memory import MemoryRecord
from orangepi_rag_service.retrieval.tokenizer import extract_terms


@dataclass(frozen=True)
class VectorSearchHit:
    memory_id: str
    score: float


class _HashingEmbeddingBackend:
    name = "hashing"
    dimension = 256

    def encode(self, texts: list[str]) -> np.ndarray:
        vectors = np.zeros((len(texts), self.dimension), dtype=np.float32)
        for row_index, text in enumerate(texts):
            tokens = extract_terms(text) or list(text)
            if not tokens:
                continue
            for token in tokens:
                digest = hashlib.sha1(token.encode("utf-8")).digest()
                slot = int.from_bytes(digest[:2], "big") % self.dimension
                sign = 1.0 if digest[2] % 2 == 0 else -1.0
                vectors[row_index, slot] += sign
            norm = np.linalg.norm(vectors[row_index])
            if norm > 0:
                vectors[row_index] /= norm
        return vectors


class _SentenceTransformerBackend:
    name = "sentence-transformer"

    def __init__(self, model_name: str) -> None:
        from sentence_transformers import SentenceTransformer

        self._model = SentenceTransformer(model_name, local_files_only=True)
        self.dimension = int(self._model.get_sentence_embedding_dimension())

    def encode(self, texts: list[str]) -> np.ndarray:
        vectors = self._model.encode(
            texts,
            normalize_embeddings=True,
            show_progress_bar=False,
        )
        return np.asarray(vectors, dtype=np.float32)


class EmbeddingIndex:
    def __init__(self, index_path: Path, model_name: str, backend_mode: str = "auto") -> None:
        self._index_path = index_path
        self._model_name = model_name
        self._backend_mode = backend_mode
        self._backend = self._build_backend()
        self._loaded_backend_name = self._backend.name
        self._vectors = np.zeros((0, 0), dtype=np.float32)
        self._ids = np.array([], dtype=str)
        self._index_path.parent.mkdir(parents=True, exist_ok=True)
        self._load_from_disk()

    def _build_backend(self):
        if self._backend_mode == "hashing":
            return _HashingEmbeddingBackend()
        if self._backend_mode == "sentence-transformer":
            return _SentenceTransformerBackend(self._model_name)
        try:
            return _SentenceTransformerBackend(self._model_name)
        except Exception:
            return _HashingEmbeddingBackend()

    @property
    def backend_name(self) -> str:
        return self._loaded_backend_name

    @property
    def degraded(self) -> bool:
        return self._loaded_backend_name != "sentence-transformer"

    def rebuild(self, records: list[MemoryRecord]) -> None:
        texts = [
            "\n".join(
                [
                    record.category,
                    record.title,
                    record.content,
                    " ".join(record.keywords),
                ]
            )
            for record in records
        ]
        ids = [record.id for record in records]
        vectors = self._backend.encode(texts) if texts else np.zeros((0, 0), dtype=np.float32)
        self._ids = np.asarray(ids, dtype=str)
        self._vectors = vectors.astype(np.float32)
        self._loaded_backend_name = self._backend.name
        np.savez(
            self._index_path,
            ids=self._ids,
            vectors=self._vectors,
            backend=np.asarray([self._loaded_backend_name]),
        )

    def search(self, query: str, limit: int = 8) -> list[VectorSearchHit]:
        if self._vectors.size == 0 or len(self._ids) == 0:
            return []
        qvec = self._backend.encode([query])
        if qvec.size == 0:
            return []
        scores = np.dot(self._vectors, qvec[0])
        indices = np.argsort(scores)[::-1][:limit]
        results: list[VectorSearchHit] = []
        for index in indices:
            score = float(scores[index])
            if score <= 0:
                continue
            results.append(VectorSearchHit(memory_id=str(self._ids[index]), score=score))
        return results

    def _load_from_disk(self) -> None:
        if not self._index_path.exists():
            return
        with np.load(self._index_path, allow_pickle=False) as data:
            self._ids = data["ids"].astype(str)
            self._vectors = data["vectors"].astype(np.float32)
            if "backend" in data:
                self._loaded_backend_name = str(data["backend"][0])
