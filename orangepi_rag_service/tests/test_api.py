from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from fastapi.testclient import TestClient

from orangepi_rag_service.app import create_app
from orangepi_rag_service.config.settings import Settings
from orangepi_rag_service.models.memory import MemoryRecord


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

    def test_admin_page(self):
        client = self._make_client()
        response = client.get("/admin")
        self.assertEqual(response.status_code, 200)
        self.assertIn("香橙派记忆管理后台", response.text)
        self.assertIn("RAG 编排", response.text)

    def test_playground_route_is_not_kept(self):
        client = self._make_client()
        response = client.get("/playground")
        self.assertEqual(response.status_code, 404)

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

    def test_list_memories_supports_search_and_inactive_filter(self):
        client = self._make_client()
        upsert_response = client.post(
            "/api/v1/memories/upsert",
            json={
                "items": [
                    {
                        "id": "admin_active_note",
                        "category": "note",
                        "title": "后台测试留言",
                        "content": "这是一条启用的后台测试记忆。",
                        "keywords": ["后台", "测试"],
                        "source": "admin",
                        "priority": 55,
                        "is_active": True,
                    },
                    {
                        "id": "admin_inactive_note",
                        "category": "note",
                        "title": "后台停用留言",
                        "content": "这是一条停用的后台测试记忆。",
                        "keywords": ["后台", "停用"],
                        "source": "admin",
                        "priority": 55,
                        "is_active": False,
                    },
                ]
            },
        )
        self.assertEqual(upsert_response.status_code, 200)

        active_response = client.get("/api/v1/memories", params={"q": "后台测试", "active": "active"})
        self.assertEqual(active_response.status_code, 200)
        active_payload = active_response.json()
        self.assertEqual(active_payload["total"], 1)
        self.assertEqual(active_payload["items"][0]["id"], "admin_active_note")

        inactive_response = client.get("/api/v1/memories", params={"active": "inactive"})
        self.assertEqual(inactive_response.status_code, 200)
        inactive_ids = {item["id"] for item in inactive_response.json()["items"]}
        self.assertIn("admin_inactive_note", inactive_ids)

    def test_import_catalog_route(self):
        client = self._make_client()
        response = client.post("/api/v1/memories/import", json={"source_type": "catalog"})
        self.assertEqual(response.status_code, 200)
        self.assertGreaterEqual(response.json()["imported"], 1)

    def test_recent_notes_deduplicates_mqtt_messages(self):
        client = self._make_client()
        upsert_payload = {
            "items": [
                {
                    "id": "note_aaaaaaaa",
                    "category": "note",
                    "title": "家属留言",
                    "content": "发送人：女儿\n内容：周末我带孩子来看你。",
                    "keywords": ["女儿", "家属留言", "来看你"],
                    "source": "mqtt_app",
                    "priority": 70,
                },
                {
                    "id": "note_bbbbbbbb",
                    "category": "note",
                    "title": "家属留言",
                    "content": "发送人：女儿\n内容：周末我带孩子来看你。",
                    "keywords": ["女儿", "家属留言", "来看你"],
                    "source": "mqtt_app",
                    "priority": 70,
                },
            ]
        }
        upsert_response = client.post("/api/v1/memories/upsert", json=upsert_payload)
        self.assertEqual(upsert_response.status_code, 200)
        self.assertEqual(upsert_response.json()["upserted"], 1)

        recent_response = client.get("/api/v1/memories/recent")
        self.assertEqual(recent_response.status_code, 200)
        payload = recent_response.json()
        self.assertEqual(payload["total"], 1)
        self.assertEqual(payload["items"][0]["category"], "note")
        self.assertTrue(payload["items"][0]["id"].startswith("note_"))

    def test_upsert_removes_existing_random_mqtt_duplicates(self):
        client = self._make_client()
        service = client.app.state.memory_service
        duplicate_content = "发送人：女儿\n内容：周末我带孩子来看你。"
        service.store.upsert_many(
            [
                MemoryRecord(
                    id="note_old1111",
                    category="note",
                    title="家属留言",
                    content=duplicate_content,
                    keywords=["女儿", "来看你"],
                    source="mqtt_app",
                    priority=70,
                ),
                MemoryRecord(
                    id="note_old2222",
                    category="note",
                    title="家属留言",
                    content=duplicate_content,
                    keywords=["女儿", "来看你"],
                    source="mqtt_app",
                    priority=70,
                ),
            ]
        )

        response = client.post(
            "/api/v1/memories/upsert",
            json={
                "item": {
                    "id": "note_new3333",
                    "category": "note",
                    "title": "家属留言",
                    "content": duplicate_content,
                    "keywords": ["女儿", "来看你"],
                    "source": "mqtt_app",
                    "priority": 70,
                }
            },
        )
        self.assertEqual(response.status_code, 200)

        recent_response = client.get("/api/v1/memories/recent")
        self.assertEqual(recent_response.status_code, 200)
        payload = recent_response.json()
        self.assertEqual(payload["total"], 1)
