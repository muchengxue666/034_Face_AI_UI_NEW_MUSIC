from __future__ import annotations

from pathlib import Path

from fastapi import APIRouter, Request
from fastapi.openapi.docs import get_swagger_ui_html
from fastapi.responses import HTMLResponse, RedirectResponse, Response


router = APIRouter()
_ADMIN_HTML_PATH = Path(__file__).resolve().parent / "admin.html"
_ADMIN_CSS_PATH = Path(__file__).resolve().parent / "admin.css"
_ADMIN_BEAUTY_CSS_PATH = Path(__file__).resolve().parent / "admin_beauty.css"
_DOCS_HEAD = """
<style>
    body {
        background: #f4f6f8;
    }

    .docs-back {
        position: fixed;
        right: 22px;
        bottom: 22px;
        z-index: 10000;
        display: inline-flex;
        align-items: center;
        justify-content: center;
        min-height: 40px;
        padding: 10px 14px;
        border-radius: 8px;
        color: #ffffff;
        background: #0f766e;
        box-shadow: 0 14px 32px rgba(15, 118, 110, 0.28);
        font-family: ui-sans-serif, -apple-system, BlinkMacSystemFont, "Segoe UI", "PingFang SC", "Microsoft YaHei", sans-serif;
        font-size: 14px;
        font-weight: 760;
        text-decoration: none;
    }

    .docs-back:hover {
        background: #0b5d54;
    }

    @media (max-width: 720px) {
        .docs-back {
            right: 14px;
            bottom: 14px;
            left: 14px;
            width: auto;
        }
    }
</style>
"""
_DOCS_BACK_LINK = '<a class="docs-back" href="/admin">返回后台</a>'
_DOCS_I18N_SCRIPT = """
<script>
    (() => {
        const translations = new Map(Object.entries({
            "Authorize": "授权",
            "Try it out": "调试",
            "Cancel": "取消",
            "Execute": "发送请求",
            "Clear": "清空",
            "Download": "下载",
            "Parameters": "参数",
            "Responses": "响应",
            "Response content type": "响应内容类型",
            "Request body": "请求体",
            "Schema": "结构",
            "Schemas": "数据结构",
            "Example Value": "示例值",
            "Model": "模型",
            "Description": "说明",
            "Default value": "默认值",
            "Required": "必填",
            "Available authorizations": "可用授权",
            "Server response": "服务端响应",
            "Code": "状态码",
            "Details": "详情",
            "Curl": "Curl 命令",
            "Request URL": "请求地址",
            "Response body": "响应体",
            "Response headers": "响应头",
            "No links": "无链接",
            "Media type": "媒体类型",
            "Controls Accept header.": "控制 Accept 请求头。",
            "Example Value | Schema": "示例值 | 结构"
        }));

        const skippedTags = new Set(["SCRIPT", "STYLE", "TEXTAREA", "INPUT", "PRE", "CODE"]);

        function translateSwaggerUi() {
            const root = document.querySelector("#swagger-ui");
            if (!root) return;
            const walker = document.createTreeWalker(root, NodeFilter.SHOW_TEXT);
            const nodes = [];
            while (walker.nextNode()) {
                nodes.push(walker.currentNode);
            }
            nodes.forEach((node) => {
                const parent = node.parentElement;
                if (!parent || skippedTags.has(parent.tagName)) return;
                const key = node.textContent.trim();
                const translated = translations.get(key);
                if (!translated) return;
                node.textContent = node.textContent.replace(key, translated);
            });
        }

        document.addEventListener("DOMContentLoaded", () => {
            translateSwaggerUi();
            const observer = new MutationObserver(translateSwaggerUi);
            observer.observe(document.body, { childList: true, subtree: true });
            setTimeout(translateSwaggerUi, 300);
            setTimeout(translateSwaggerUi, 1000);
        });
    })();
</script>
"""


@router.get("/", include_in_schema=False)
def index() -> RedirectResponse:
    return RedirectResponse(url="/admin", status_code=307)


@router.get("/admin", response_class=HTMLResponse, include_in_schema=False)
def admin() -> HTMLResponse:
    return HTMLResponse(_ADMIN_HTML_PATH.read_text(encoding="utf-8"))


@router.get("/admin.css", include_in_schema=False)
def admin_css() -> Response:
    return Response(_ADMIN_CSS_PATH.read_text(encoding="utf-8"), media_type="text/css")


@router.get("/admin_beauty.css", include_in_schema=False)
def admin_beauty_css() -> Response:
    return Response(_ADMIN_BEAUTY_CSS_PATH.read_text(encoding="utf-8"), media_type="text/css")


@router.get("/docs", response_class=HTMLResponse, include_in_schema=False)
def docs() -> HTMLResponse:
    response = get_swagger_ui_html(
        openapi_url="/openapi.json",
        title="香橙派家庭记忆中枢 - 接口文档",
        swagger_ui_parameters={"docExpansion": "none", "persistAuthorization": True},
    )
    html = response.body.decode("utf-8")
    html = html.replace("</head>", f"{_DOCS_HEAD}</head>")
    html = html.replace("<body>", f"<body>{_DOCS_BACK_LINK}", 1)
    html = html.replace("</body>", f"{_DOCS_I18N_SCRIPT}</body>")
    return HTMLResponse(html)


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
