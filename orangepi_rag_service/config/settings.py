from __future__ import annotations

import os
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class Settings:
    base_dir: Path
    data_dir: Path
    db_path: Path
    vector_index_path: Path
    catalog_path: Path
    legacy_memory_path: Path
    service_host: str
    service_port: int
    embedding_model: str
    embedding_backend: str
    default_session_mode: str
    max_prompt_chars: int
    max_summary_chars: int
    max_hits: int

    @classmethod
    def load(cls) -> "Settings":
        base_dir = Path(__file__).resolve().parent.parent
        data_dir = base_dir / "data"
        return cls(
            base_dir=base_dir,
            data_dir=data_dir,
            db_path=data_dir / "memory.db",
            vector_index_path=data_dir / "vector_index.npz",
            catalog_path=base_dir / "memory_catalog.yaml",
            legacy_memory_path=base_dir / "family_memory.txt",
            service_host=os.getenv("ORANGEPI_MEMORY_HOST", "0.0.0.0"),
            service_port=int(os.getenv("ORANGEPI_MEMORY_PORT", "8765")),
            embedding_model=os.getenv(
                "ORANGEPI_MEMORY_EMBED_MODEL",
                "shibing624/text2vec-base-chinese",
            ),
            embedding_backend=os.getenv("ORANGEPI_MEMORY_EMBED_BACKEND", "hashing"),
            default_session_mode="elder_companion",
            max_prompt_chars=1200,
            max_summary_chars=700,
            max_hits=4,
        )

    def ensure_directories(self) -> None:
        self.data_dir.mkdir(parents=True, exist_ok=True)
