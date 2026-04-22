from __future__ import annotations

from typing import Optional

from pydantic import BaseModel, Field


class RagComposeRequest(BaseModel):
    query: str = Field(min_length=1)
    session_mode: str = "elder_companion"
    max_prompt_chars: int = Field(default=1200, ge=400, le=1200)


class MemoryPayload(BaseModel):
    id: Optional[str] = None
    category: str
    title: str
    content: str
    keywords: list[str] = Field(default_factory=list)
    source: str = "api"
    priority: int = 50
    effective_at: Optional[str] = None
    expires_at: Optional[str] = None
    is_active: bool = True
    updated_at: Optional[str] = None


class MemoryHitResponse(BaseModel):
    id: str
    category: str
    title: str
    summary: str
    score: float
    source: str


class RagComposeResponse(BaseModel):
    system_prompt: str
    intent: str
    hits: list[MemoryHitResponse]
    trace_id: str
    elapsed_ms: int
    degraded: bool
    has_memory: bool


class UpsertMemoriesRequest(BaseModel):
    item: Optional[MemoryPayload] = None
    items: list[MemoryPayload] = Field(default_factory=list)


class UpsertMemoriesResponse(BaseModel):
    status: str
    upserted: int
    index_version: str


class ImportMemoriesRequest(BaseModel):
    source_type: str = "catalog"
    path: Optional[str] = None
    persist_catalog: bool = True


class ImportMemoriesResponse(BaseModel):
    status: str
    imported: int
    index_version: str


class HealthResponse(BaseModel):
    status: str
    index_version: str
    memory_count: int
    last_rebuild_time: str
    embedding_backend: str
    degraded: bool
