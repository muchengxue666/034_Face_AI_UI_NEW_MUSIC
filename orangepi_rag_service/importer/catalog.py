from __future__ import annotations

from pathlib import Path

import yaml

from orangepi_rag_service.models.memory import MemoryRecord


def load_catalog(path: Path) -> list[MemoryRecord]:
    raw = yaml.safe_load(path.read_text(encoding="utf-8")) or {}
    items = raw.get("memories", [])
    if not isinstance(items, list):
        raise ValueError("memory_catalog.yaml 中的 memories 必须是数组")
    return [MemoryRecord.from_mapping(item, default_source="catalog") for item in items]


def write_catalog(path: Path, records: list[MemoryRecord]) -> None:
    path.write_text(
        yaml.safe_dump(
            {
                "version": 1,
                "memories": [record.as_dict() for record in records],
            },
            allow_unicode=True,
            sort_keys=False,
        ),
        encoding="utf-8",
    )
