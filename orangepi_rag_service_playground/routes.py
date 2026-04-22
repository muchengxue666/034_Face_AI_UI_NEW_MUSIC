from __future__ import annotations

from pathlib import Path

from fastapi import APIRouter, Request
from fastapi.responses import HTMLResponse, RedirectResponse


router = APIRouter()
_PLAYGROUND_HTML_PATH = Path(__file__).resolve().parent / "playground.html"


@router.get("/", include_in_schema=False)
def index() -> RedirectResponse:
    return RedirectResponse(url="/playground", status_code=307)


@router.get("/playground", response_class=HTMLResponse, include_in_schema=False)
def playground() -> HTMLResponse:
    return HTMLResponse(_PLAYGROUND_HTML_PATH.read_text(encoding="utf-8"))


@router.get("/api/v1/debug/catalog", include_in_schema=False)
def debug_catalog(request: Request):
    service = request.app.state.memory_service
    catalog_path = service.settings.catalog_path
    legacy_path = service.settings.legacy_memory_path
    return {
        "catalog_path": str(catalog_path),
        "catalog_exists": catalog_path.exists(),
        "catalog_content": catalog_path.read_text(encoding="utf-8") if catalog_path.exists() else "",
        "legacy_path": str(legacy_path),
        "legacy_exists": legacy_path.exists(),
    }
