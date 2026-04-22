from __future__ import annotations

from dataclasses import dataclass, field
from datetime import datetime, timezone
from typing import Iterable, Optional
import hashlib


ALLOWED_CATEGORIES = {
    "profile",
    "family",
    "health",
    "medication",
    "diet",
    "routine",
    "preference",
    "event",
    "contact",
    "safety",
    "note",
}


def utc_now_iso() -> str:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat()


def normalize_keywords(values: Optional[Iterable[str]]) -> list[str]:
    if not values:
        return []
    result: list[str] = []
    seen: set[str] = set()
    for raw in values:
        text = str(raw).strip()
        if not text:
            continue
        if text not in seen:
            seen.add(text)
            result.append(text)
    return result


def ensure_memory_id(category: str, title: str, content: str, supplied_id: Optional[str] = None) -> str:
    if supplied_id:
        return supplied_id.strip()
    digest = hashlib.sha1(f"{category}|{title}|{content}".encode("utf-8")).hexdigest()[:12]
    return f"{category}_{digest}"


@dataclass(slots=True)
class MemoryRecord:
    id: str
    category: str
    title: str
    content: str
    keywords: list[str] = field(default_factory=list)
    source: str = "catalog"
    priority: int = 50
    effective_at: Optional[str] = None
    expires_at: Optional[str] = None
    is_active: bool = True
    updated_at: str = field(default_factory=utc_now_iso)

    @classmethod
    def from_mapping(cls, payload: dict, default_source: str = "catalog") -> "MemoryRecord":
        category = str(payload.get("category", "")).strip()
        if category not in ALLOWED_CATEGORIES:
            raise ValueError(f"不支持的记忆分类: {category}")

        title = str(payload.get("title", "")).strip()
        content = str(payload.get("content", "")).strip()
        if not title:
            raise ValueError("title 不能为空")
        if not content:
            raise ValueError("content 不能为空")

        return cls(
            id=ensure_memory_id(category, title, content, payload.get("id")),
            category=category,
            title=title,
            content=content,
            keywords=normalize_keywords(payload.get("keywords")),
            source=str(payload.get("source") or default_source).strip(),
            priority=int(payload.get("priority", 50)),
            effective_at=payload.get("effective_at"),
            expires_at=payload.get("expires_at"),
            is_active=bool(payload.get("is_active", True)),
            updated_at=str(payload.get("updated_at") or utc_now_iso()),
        )

    def as_dict(self) -> dict:
        return {
            "id": self.id,
            "category": self.category,
            "title": self.title,
            "content": self.content,
            "keywords": list(self.keywords),
            "source": self.source,
            "priority": self.priority,
            "effective_at": self.effective_at,
            "expires_at": self.expires_at,
            "is_active": self.is_active,
            "updated_at": self.updated_at,
        }


@dataclass(slots=True)
class MemoryHit:
    record: MemoryRecord
    score: float
    source: str
