from __future__ import annotations

from pathlib import Path

from fastapi import APIRouter, HTTPException, Query, Request

from orangepi_rag_service.models.api import (
    HealthResponse,
    ImportMemoriesRequest,
    ImportMemoriesResponse,
    MemoriesListResponse,
    RagComposeRequest,
    RagComposeResponse,
    RecentMemoriesResponse,
    UpsertMemoriesRequest,
    UpsertMemoriesResponse,
)
from orangepi_rag_service.models.memory import MemoryRecord


router = APIRouter(prefix="/api/v1")


def _get_service(request: Request):
    return request.app.state.memory_service


def _model_dump(model) -> dict:
    if hasattr(model, "model_dump"):
        return model.model_dump(exclude_none=True)
    return model.dict(exclude_none=True)


@router.get(
    "/health",
    response_model=HealthResponse,
    tags=["系统状态"],
    summary="查看服务健康状态",
    description="返回当前记忆数量、索引版本、最近重建时间和向量引擎状态。",
)
def health(request: Request):
    service = _get_service(request)
    return service.health()


@router.post(
    "/rag/compose",
    response_model=RagComposeResponse,
    tags=["RAG 编排"],
    summary="组合 RAG 系统提示词",
    description="输入用户问题后，服务会判断是否需要记忆检索，并返回可直接传给大模型的系统提示词。",
)
def compose_rag(request: Request, payload: RagComposeRequest):
    service = _get_service(request)
    try:
        return service.compose(payload.query, payload.session_mode, payload.max_prompt_chars)
    except ValueError as exc:
        raise HTTPException(status_code=400, detail=str(exc)) from exc


@router.post(
    "/memories/upsert",
    response_model=UpsertMemoriesResponse,
    tags=["记忆管理"],
    summary="新增或更新记忆",
    description="按记忆 ID 新增或覆盖记忆；未提供 ID 时会根据分类、标题和内容生成稳定 ID。",
)
def upsert_memories(request: Request, payload: UpsertMemoriesRequest):
    service = _get_service(request)
    items = list(payload.items)
    if payload.item is not None:
        items.insert(0, payload.item)
    if not items:
        raise HTTPException(status_code=400, detail="item 或 items 至少提供一个")

    try:
        records = [MemoryRecord.from_mapping(_model_dump(item), default_source="api") for item in items]
        upserted = service.upsert_memories(records)
    except ValueError as exc:
        raise HTTPException(status_code=400, detail=str(exc)) from exc

    return {
        "status": "ok",
        "upserted": upserted,
        "index_version": service.health()["index_version"],
    }


@router.get(
    "/memories",
    response_model=MemoriesListResponse,
    tags=["记忆管理"],
    summary="查询记忆列表",
    description="支持按分类、关键词和启用状态筛选记忆，主要供后台管理页使用。",
)
def list_memories(
    request: Request,
    category: str | None = Query(default=None, description="记忆分类，例如 note、health、family。为空时返回全部分类。"),
    q: str | None = Query(default=None, description="搜索关键词，会匹配标题、内容和关键词。"),
    active: str = Query(default="active", description="启用状态筛选：active、inactive 或 all。"),
    limit: int = Query(default=100, ge=1, le=200, description="最多返回的记忆条数。"),
):
    service = _get_service(request)
    try:
        items = service.list_memories(category=category, query=q, active_state=active, limit=limit)
    except ValueError as exc:
        raise HTTPException(status_code=400, detail=str(exc)) from exc
    return {"items": items, "total": len(items)}


@router.get(
    "/memories/recent",
    response_model=RecentMemoriesResponse,
    tags=["记忆管理"],
    summary="查看最近记忆",
    description="返回最近更新的记忆，默认只查看 note 分类，适合读取家属留言等最新内容。",
)
def recent_memories(
    request: Request,
    category: str | None = Query(default="note", description="记忆分类。传空值时返回全部分类。"),
    limit: int = Query(default=20, ge=1, le=100, description="最多返回的记忆条数。"),
):
    service = _get_service(request)
    try:
        items = service.list_recent_memories(category=category, limit=limit)
    except ValueError as exc:
        raise HTTPException(status_code=400, detail=str(exc)) from exc
    return {"items": items, "total": len(items)}


@router.post(
    "/memories/import",
    response_model=ImportMemoriesResponse,
    tags=["记忆管理"],
    summary="导入记忆文件",
    description="从 YAML 目录或旧版文本文件导入记忆，并重建检索索引。",
)
def import_memories(request: Request, payload: ImportMemoriesRequest):
    service = _get_service(request)
    source_type = payload.source_type.strip().lower()
    path = Path(payload.path) if payload.path else None

    try:
        if source_type == "catalog":
            imported = service.import_catalog(path or service.settings.catalog_path)
        elif source_type == "legacy_text":
            imported = service.import_legacy_text(path or service.settings.legacy_memory_path, persist_catalog=payload.persist_catalog)
        else:
            raise HTTPException(status_code=400, detail=f"不支持的 source_type: {source_type}")
    except FileNotFoundError as exc:
        raise HTTPException(status_code=404, detail=str(exc)) from exc
    except ValueError as exc:
        raise HTTPException(status_code=400, detail=str(exc)) from exc

    return {
        "status": "ok",
        "imported": imported,
        "index_version": service.health()["index_version"],
    }
