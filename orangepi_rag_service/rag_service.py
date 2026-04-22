"""
兼容启动入口。

旧的 LangChain/Chroma 单文件实现已废弃，
现在统一转发到新的香橙派家庭记忆中枢应用。
"""

from orangepi_rag_service.app import run


if __name__ == "__main__":
    run()
