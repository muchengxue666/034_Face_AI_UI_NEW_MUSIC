from __future__ import annotations

from fastapi import FastAPI
import uvicorn

from orangepi_rag_service.api.routes import router
from orangepi_rag_service.config.settings import Settings
from orangepi_rag_service.service import MemoryOrchestrator
from orangepi_rag_service.web.routes import router as web_router


_OPENAPI_TAGS = [
    {"name": "系统状态", "description": "查看服务健康状态、索引版本和向量引擎状态。"},
    {"name": "RAG 编排", "description": "根据用户问题组合系统提示词，并返回命中的记忆片段。"},
    {"name": "记忆管理", "description": "新增、查询、导入和查看家庭记忆数据。"},
]


def create_app(settings: Settings | None = None) -> FastAPI:
    settings = settings or Settings.load()
    service = MemoryOrchestrator(settings)
    service.bootstrap()

    app = FastAPI(
        title="香橙派家庭记忆中枢 API",
        description="面向 ESP32 语音终端和后台管理页的家庭记忆检索、RAG 编排与记忆维护接口。",
        version="2.0.0",
        docs_url=None,
        redoc_url=None,
        openapi_tags=_OPENAPI_TAGS,
    )
    app.state.memory_service = service
    app.include_router(router)
    app.include_router(web_router)
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
