from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from orangepi_rag_service.importer.catalog import load_catalog, write_catalog
from orangepi_rag_service.importer.legacy_text import parse_legacy_text


class ImporterTests(unittest.TestCase):
    def test_parse_legacy_text(self):
        records = parse_legacy_text(Path("orangepi_rag_service/family_memory.txt"))
        self.assertGreaterEqual(len(records), 10)
        self.assertEqual(records[0].category, "profile")

    def test_catalog_roundtrip(self):
        records = parse_legacy_text(Path("orangepi_rag_service/family_memory.txt"))
        with tempfile.TemporaryDirectory() as temp_dir:
            target = Path(temp_dir) / "memory_catalog.yaml"
            write_catalog(target, records)
            loaded = load_catalog(target)
        self.assertEqual(len(records), len(loaded))
        self.assertEqual(records[0].title, loaded[0].title)
