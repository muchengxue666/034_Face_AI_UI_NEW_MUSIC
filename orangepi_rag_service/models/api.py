from __future__ import annotations

from typing import Optional

from pydantic import BaseModel, Field


class RagComposeRequest(BaseModel):
    query: str = Field(min_length=1, description="用户当前说的话或提问内容。")
    session_mode: str = Field(default="elder_companion", description="会话模式，默认使用长者陪伴模式。")
    max_prompt_chars: int = Field(default=1200, ge=400, le=1200, description="系统提示词最大字符数。")

    class Config:
        title = "RAG 提示词请求"


class MemoryPayload(BaseModel):
    id: Optional[str] = Field(default=None, description="记忆 ID。为空时由服务自动生成。")
    category: str = Field(description="记忆分类，例如 profile、family、health、note。")
    title: str = Field(description="记忆标题，用于后台列表和检索摘要。")
    content: str = Field(description="记忆正文，是 RAG 检索和提示词拼接的主要内容。")
    keywords: list[str] = Field(default_factory=list, description="辅助检索关键词。")
    source: str = Field(default="api", description="记忆来源，例如 api、admin、mqtt_app、catalog。")
    priority: int = Field(default=50, description="记忆优先级，数值越高越优先。")
    effective_at: Optional[str] = Field(default=None, description="生效时间，建议使用 ISO 8601 字符串。")
    expires_at: Optional[str] = Field(default=None, description="过期时间，建议使用 ISO 8601 字符串。")
    is_active: bool = Field(default=True, description="是否启用该记忆。")
    updated_at: Optional[str] = Field(default=None, description="更新时间。为空时由服务写入当前时间。")

    class Config:
        title = "记忆载荷"


class MemoryRecordResponse(MemoryPayload):
    id: str = Field(description="记忆 ID。")
    updated_at: str = Field(description="最近更新时间。")

    class Config:
        title = "记忆记录"


class MemoryHitResponse(BaseModel):
    id: str = Field(description="命中的记忆 ID。")
    category: str = Field(description="命中的记忆分类。")
    title: str = Field(description="命中的记忆标题。")
    summary: str = Field(description="用于提示词的记忆摘要。")
    score: float = Field(description="检索相关度分数。")
    source: str = Field(description="命中来源，例如 keyword 或 vector。")

    class Config:
        title = "记忆命中"


class RagComposeResponse(BaseModel):
    system_prompt: str = Field(description="组合后的系统提示词。")
    intent: str = Field(description="意图分类结果。")
    hits: list[MemoryHitResponse] = Field(description="本次命中的记忆列表。")
    trace_id: str = Field(description="请求追踪 ID。")
    elapsed_ms: int = Field(description="处理耗时，单位毫秒。")
    degraded: bool = Field(description="向量检索是否处于降级模式。")
    has_memory: bool = Field(description="本次回答是否使用了记忆。")

    class Config:
        title = "RAG 提示词响应"


class UpsertMemoriesRequest(BaseModel):
    item: Optional[MemoryPayload] = Field(default=None, description="单条记忆。")
    items: list[MemoryPayload] = Field(default_factory=list, description="批量记忆列表。")

    class Config:
        title = "写入记忆请求"


class UpsertMemoriesResponse(BaseModel):
    status: str = Field(description="处理结果，成功时为 ok。")
    upserted: int = Field(description="实际写入的记忆条数。")
    index_version: str = Field(description="写入后的索引版本。")

    class Config:
        title = "写入记忆响应"


class MemoriesListResponse(BaseModel):
    items: list[MemoryRecordResponse] = Field(description="记忆列表。")
    total: int = Field(description="返回条数。")

    class Config:
        title = "记忆列表响应"


class RecentMemoriesResponse(MemoriesListResponse):
    class Config:
        title = "最近记忆响应"


class ImportMemoriesRequest(BaseModel):
    source_type: str = Field(default="catalog", description="导入来源类型：catalog 或 legacy_text。")
    path: Optional[str] = Field(default=None, description="导入文件路径。为空时使用配置里的默认路径。")
    persist_catalog: bool = Field(default=True, description="从旧版文本导入时，是否同时写出 YAML 目录文件。")

    class Config:
        title = "导入记忆请求"


class ImportMemoriesResponse(BaseModel):
    status: str = Field(description="处理结果，成功时为 ok。")
    imported: int = Field(description="导入的记忆条数。")
    index_version: str = Field(description="导入后的索引版本。")

    class Config:
        title = "导入记忆响应"


class HealthResponse(BaseModel):
    status: str = Field(description="服务状态，正常时为 ok。")
    index_version: str = Field(description="当前索引版本。")
    memory_count: int = Field(description="当前记忆总数。")
    last_rebuild_time: str = Field(description="最近一次索引重建时间。")
    embedding_backend: str = Field(description="当前使用的向量引擎。")
    degraded: bool = Field(description="是否处于降级检索模式。")

    class Config:
        title = "健康状态响应"
