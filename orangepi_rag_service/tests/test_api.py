from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from fastapi.testclient import TestClient

from orangepi_rag_service.app import create_app
from orangepi_rag_service.config.settings import Settings


class ApiTests(unittest.TestCase):
    def _make_client(self) -> TestClient:
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
        client = TestClient(create_app(settings))
        self.addCleanup(client.close)
        return client

    def test_health(self):
        client = self._make_client()
        response = client.get("/api/v1/health")
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.json()["status"], "ok")

    def test_playground_page(self):
        client = self._make_client()
        response = client.get("/playground")
        self.assertEqual(response.status_code, 200)
        self.assertIn("香橙派记忆中枢本地测试台", response.text)

    def test_debug_catalog(self):
        client = self._make_client()
        response = client.get("/api/v1/debug/catalog")
        self.assertEqual(response.status_code, 200)
        payload = response.json()
        self.assertTrue(payload["catalog_exists"])
        self.assertIn("memories:", payload["catalog_content"])

    def test_upsert_and_compose(self):
        client = self._make_client()
        upsert_payload = {
            "items": [
                {
                    "category": "note",
                    "title": "家属留言",
                    "content": "周末我带孩子来看你。",
                    "keywords": ["周末", "来看你"],
                    "source": "mqtt",
                    "priority": 70,
                }
            ]
        }
        upsert_response = client.post("/api/v1/memories/upsert", json=upsert_payload)
        self.assertEqual(upsert_response.status_code, 200)

        compose_response = client.post(
            "/api/v1/rag/compose",
            json={"query": "家里人什么时候来看我", "session_mode": "elder_companion", "max_prompt_chars": 1200},
        )
        self.assertEqual(compose_response.status_code, 200)
        self.assertTrue(compose_response.json()["has_memory"])
