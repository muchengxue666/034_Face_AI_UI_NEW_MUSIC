from __future__ import annotations

from fastapi import FastAPI
import uvicorn

from orangepi_rag_service.api.routes import router
from orangepi_rag_service.config.settings import Settings
from orangepi_rag_service.service import MemoryOrchestrator

try:
    from orangepi_rag_service_playground.routes import router as playground_router
except ModuleNotFoundError:
    playground_router = None


def create_app(settings: Settings | None = None) -> FastAPI:
    settings = settings or Settings.load()
    service = MemoryOrchestrator(settings)
    service.bootstrap()

    app = FastAPI(title="香橙派家庭记忆中枢", version="2.0.0")
    app.state.memory_service = service
    app.include_router(router)
    if playground_router is not None:
        app.include_router(playground_router)
    return app


app = create_app()


def run() -> None:
    settings = Settings.load()
    uvicorn.run(
        "orangepi_rag_service.app:app",
        host=settings.service_host,
        port=settings.service_port,
        log_level="info",
    )
