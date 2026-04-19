"""
RAG 记忆服务 - 运行在香橙派 AIPro 上
=====================================================
功能：
  1. 加载 family_memory.txt（家庭生活记录）
  2. 用 LangChain 切片 + 向量化，存入 ChromaDB
  3. 提供 HTTP /query 接口，供 ESP32-P4 调用
  4. 返回：与用户问题最相关的记忆片段（用于注入豆包 system prompt）

安装依赖（香橙派上运行）：
  pip install fastapi uvicorn langchain langchain-community \
              chromadb sentence-transformers

启动：
  python rag_service.py
  或后台运行：
  nohup python rag_service.py > rag.log 2>&1 &
"""

import os
import json
import time
import logging
from pathlib import Path
from typing import Optional

from fastapi import FastAPI, HTTPException
from fastapi.responses import JSONResponse
from pydantic import BaseModel
import uvicorn

from langchain_community.document_loaders import TextLoader
from langchain.text_splitter import RecursiveCharacterTextSplitter
from langchain_community.vectorstores import Chroma

# ==================== 配置 ====================
MEMORY_FILE     = "./family_memory.txt"   # 家庭记忆文件路径
CHROMA_DIR      = "./chroma_db"           # 向量数据库持久化目录
EMBED_MODEL     = "shibing624/text2vec-base-chinese"  # 中文向量模型（本地运行）
SERVICE_HOST    = "0.0.0.0"
SERVICE_PORT    = 8765                    # ESP32 访问这个端口
TOP_K           = 3                       # 每次检索返回最相关的3段记忆
CHUNK_SIZE      = 300                     # 每段切片最大字符数
CHUNK_OVERLAP   = 50                      # 相邻切片重叠字符数（保证上下文连贯）

# ==================== 日志 ====================
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s"
)
log = logging.getLogger("RAG")

# ==================== 全局向量库 ====================
vectorstore: Optional[Chroma] = None
embeddings = None  # 向量嵌入模型

# ==================== 启动时加载 ====================

def load_and_index():
    """加载 family_memory.txt，切片，向量化，写入 ChromaDB"""
    global vectorstore, embeddings

    if not Path(MEMORY_FILE).exists():
        log.warning(f"记忆文件不存在：{MEMORY_FILE}，请先创建它")
        return False

    log.info("正在加载向量嵌入模型（首次运行会下载，约400MB）...")
    try:
        from langchain_huggingface import HuggingFaceEmbeddings
        embeddings = HuggingFaceEmbeddings(
            model_name=EMBED_MODEL,
            model_kwargs={"device": "cpu"},   # 香橙派 AIPro 用 NPU 可改为 "npu"
            encode_kwargs={"normalize_embeddings": True},
        )
    except ImportError:
        # 降级使用旧版本
        from langchain_community.embeddings import HuggingFaceEmbeddings
        embeddings = HuggingFaceEmbeddings(
            model_name=EMBED_MODEL,
            model_kwargs={"device": "cpu"},   # 香橙派 AIPro 用 NPU 可改为 "npu"
            encode_kwargs={"normalize_embeddings": True},
        )
    log.info("向量模型加载完成")

    # 如果向量库已存在且记忆文件未修改，直接复用
    chroma_path = Path(CHROMA_DIR)
    memory_mtime = Path(MEMORY_FILE).stat().st_mtime
    mtime_cache  = Path(CHROMA_DIR) / ".mtime"

    if chroma_path.exists() and mtime_cache.exists():
        cached_mtime = float(mtime_cache.read_text().strip())
        if abs(cached_mtime - memory_mtime) < 1.0:
            log.info("记忆文件未变化，直接加载已有向量库...")
            vectorstore = Chroma(
                persist_directory=CHROMA_DIR,
                embedding_function=embeddings,
            )
            log.info(f"向量库加载完成，共 {vectorstore._collection.count()} 个片段")
            return True

    # 重新构建向量库
    log.info(f"正在读取记忆文件：{MEMORY_FILE}")
    loader = TextLoader(MEMORY_FILE, encoding="utf-8")
    docs   = loader.load()

    splitter = RecursiveCharacterTextSplitter(
        chunk_size=CHUNK_SIZE,
        chunk_overlap=CHUNK_OVERLAP,
        separators=["\n\n", "\n", "。", "，", " ", ""],
    )
    chunks = splitter.split_documents(docs)
    log.info(f"文档切分完成，共 {len(chunks)} 个片段")

    # 向量化并写入 ChromaDB
    log.info("正在向量化，请稍候...")
    vectorstore = Chroma.from_documents(
        documents=chunks,
        embedding=embeddings,
        persist_directory=CHROMA_DIR,
    )
    vectorstore.persist()

    # 记录文件修改时间，下次启动可以跳过重建
    Path(CHROMA_DIR).mkdir(exist_ok=True)
    mtime_cache.write_text(str(memory_mtime))

    log.info(f"向量库构建完成，共 {len(chunks)} 个片段已索引")
    return True


def reload_memory():
    """热重载：记忆文件更新后调用此函数重建向量库"""
    global vectorstore
    # 删除 mtime 缓存，强制重建
    mtime_cache = Path(CHROMA_DIR) / ".mtime"
    if mtime_cache.exists():
        mtime_cache.unlink()
    log.info("开始热重载记忆文件...")
    return load_and_index()


# ==================== FastAPI 应用 ====================
app = FastAPI(title="家庭记忆 RAG 服务", version="1.0")


class QueryRequest(BaseModel):
    question: str          # ESP32 发来的用户问题（STT 识别文本）
    top_k: int = TOP_K     # 可选：返回几段记忆


class QueryResponse(BaseModel):
    context: str           # 拼接好的记忆片段，直接注入 system prompt
    sources: list[str]     # 各片段原文（调试用）
    elapsed_ms: int        # 检索耗时


@app.on_event("startup")
async def startup():
    log.info("RAG 服务启动中...")
    success = load_and_index()
    if not success:
        log.warning("记忆文件加载失败，服务将以无记忆模式运行")


@app.get("/health")
async def health():
    """ESP32 可以先 ping 这个接口确认服务可用"""
    count = 0
    if vectorstore:
        count = vectorstore._collection.count()
    return {"status": "ok", "indexed_chunks": count}


@app.post("/query", response_model=QueryResponse)
async def query_memory(req: QueryRequest):
    """
    ESP32 STT 识别出问题后调用此接口
    返回最相关的记忆片段，供 ESP32 注入豆包 system prompt
    """
    if not vectorstore:
        # 服务未就绪，返回空记忆（ESP32 会用无记忆模式降级）
        return QueryResponse(context="", sources=[], elapsed_ms=0)

    if not req.question or len(req.question.strip()) == 0:
        raise HTTPException(status_code=400, detail="question 不能为空")

    t0 = time.time()

    # 相似度检索
    results = vectorstore.similarity_search_with_score(
        query=req.question,
        k=min(req.top_k, 5),
    )

    # 过滤相关性太低的结果（score 越小越相似，阈值 1.2）
    relevant = [(doc, score) for doc, score in results if score < 1.2]

    if not relevant:
        return QueryResponse(context="", sources=[], elapsed_ms=int((time.time()-t0)*1000))

    # 拼接记忆片段
    context_parts = []
    sources       = []
    for doc, score in relevant:
        text = doc.page_content.strip()
        context_parts.append(text)
        sources.append(f"[相关度:{1-score/2:.0%}] {text[:60]}...")

    # 拼成一段，给豆包的 system prompt 用
    context = "【关于这个家庭的记忆】\n" + "\n---\n".join(context_parts)

    elapsed = int((time.time() - t0) * 1000)
    log.info(f"查询: '{req.question[:30]}...' → {len(relevant)} 段记忆，耗时 {elapsed}ms")

    return QueryResponse(context=context, sources=sources, elapsed_ms=elapsed)


@app.post("/reload")
async def reload():
    """记忆文件更新后，调用此接口重建向量库（无需重启服务）"""
    success = reload_memory()
    if success:
        return {"status": "ok", "message": "记忆重载成功"}
    else:
        raise HTTPException(status_code=500, detail="重载失败，请检查记忆文件")


@app.get("/stats")
async def stats():
    """查看向量库统计信息"""
    if not vectorstore:
        return {"status": "not_ready"}
    count = vectorstore._collection.count()
    memory_size = Path(MEMORY_FILE).stat().st_size if Path(MEMORY_FILE).exists() else 0
    return {
        "status": "ready",
        "indexed_chunks": count,
        "memory_file_size_bytes": memory_size,
        "memory_file": MEMORY_FILE,
        "embed_model": EMBED_MODEL,
    }


# ==================== 主入口 ====================
if __name__ == "__main__":
    log.info(f"RAG 服务监听 {SERVICE_HOST}:{SERVICE_PORT}")
    log.info(f"ESP32 访问地址：http://<香橙派IP>:{SERVICE_PORT}/query")
    uvicorn.run(app, host=SERVICE_HOST, port=SERVICE_PORT, log_level="info")
