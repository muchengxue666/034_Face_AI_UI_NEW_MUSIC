from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from orangepi_rag_service.config.settings import Settings
from orangepi_rag_service.service import MemoryOrchestrator


class RetrievalTests(unittest.TestCase):
    def _make_service(self) -> MemoryOrchestrator:
        temp_dir = tempfile.TemporaryDirectory()
        self.addCleanup(temp_dir.cleanup)
        base_dir = Path(temp_dir.name)
        settings = Settings(
            base_dir=base_dir,
            data_dir=base_dir / "data",
            db_path=base_dir / "data" / "memory.db",
            vector_index_path=base_dir / "data" / "vector_index.npz",
            catalog_path=Path("orangepi_rag_service/memory_catalog.yaml"),
            legacy_memory_path=Path("orangepi_rag_service/family_memory.txt"),
            service_host="127.0.0.1",
            service_port=8765,
            embedding_model="dummy",
            embedding_backend="hashing",
            default_session_mode="elder_companion",
            max_prompt_chars=1200,
            max_summary_chars=700,
            max_hits=4,
        )
        service = MemoryOrchestrator(settings)
        service.bootstrap()
        return service

    def test_compose_medication_hits(self):
        service = self._make_service()
        result = service.compose("我早上几点吃降压药", "elder_companion", 1200)
        self.assertTrue(result["has_memory"])
        self.assertEqual(result["intent"], "medication")
        self.assertGreaterEqual(len(result["hits"]), 1)

    def test_compose_unknown_query_falls_back(self):
        service = self._make_service()
        result = service.compose("今天外面会不会下雪", "elder_companion", 1200)
        self.assertFalse(result["has_memory"])
        self.assertIn("不要编造", result["system_prompt"])
