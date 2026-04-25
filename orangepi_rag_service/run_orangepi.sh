#!/usr/bin/env bash
set -euo pipefail

prepare_only=0
if [[ "${1:-}" == "--prepare-only" ]]; then
    prepare_only=1
fi

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "${script_dir}/.." && pwd)"
venv_dir="${ORANGEPI_MEMORY_VENV:-${repo_root}/.venv-orangepi}"
python_bin="${PYTHON_BIN:-python3}"

export ORANGEPI_MEMORY_HOST="${ORANGEPI_MEMORY_HOST:-0.0.0.0}"
export ORANGEPI_MEMORY_PORT="${ORANGEPI_MEMORY_PORT:-8765}"
export ORANGEPI_MEMORY_EMBED_BACKEND="${ORANGEPI_MEMORY_EMBED_BACKEND:-hashing}"
export PYTHONUNBUFFERED="${PYTHONUNBUFFERED:-1}"

cd "${repo_root}"

if [[ ! -x "${venv_dir}/bin/python" ]]; then
    echo "创建香橙派运行虚拟环境: ${venv_dir}"
    "${python_bin}" -m venv "${venv_dir}"
    "${venv_dir}/bin/python" -m pip install --upgrade pip
    "${venv_dir}/bin/pip" install -r "${script_dir}/requirements-orangepi.txt"
fi

if [[ "${prepare_only}" == "1" ]]; then
    echo "香橙派运行环境准备完成"
    exit 0
fi

echo "启动香橙派记忆中枢: http://${ORANGEPI_MEMORY_HOST}:${ORANGEPI_MEMORY_PORT}"
echo "向量后端: ${ORANGEPI_MEMORY_EMBED_BACKEND}"

exec "${venv_dir}/bin/python" -m uvicorn orangepi_rag_service.app:app \
    --host "${ORANGEPI_MEMORY_HOST}" \
    --port "${ORANGEPI_MEMORY_PORT}"
