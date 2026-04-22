from __future__ import annotations

from fastapi import APIRouter, HTTPException, Request

from orangepi_rag_service.models.api import (
    HealthResponse,
    ImportMemoriesRequest,
    ImportMemoriesResponse,
    RagComposeRequest,
    RagComposeResponse,
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


@router.get("/health", response_model=HealthResponse)
def health(request: Request):
    service = _get_service(request)
    return service.health()


@router.post("/rag/compose", response_model=RagComposeResponse)
def compose_rag(request: Request, payload: RagComposeRequest):
    service = _get_service(request)
    try:
        return service.compose(payload.query, payload.session_mode, payload.max_prompt_chars)
    except ValueError as exc:
        raise HTTPException(status_code=400, detail=str(exc)) from exc


@router.post("/memories/upsert", response_model=UpsertMemoriesResponse)
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


@router.post("/memories/import", response_model=ImportMemoriesResponse)
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

