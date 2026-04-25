#!/usr/bin/env bash
set -euo pipefail

if [[ "$(id -u)" -ne 0 ]]; then
    echo "请使用 sudo 执行: sudo bash orangepi_rag_service/install_orangepi_service.sh"
    exit 1
fi

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "${script_dir}/.." && pwd)"
run_script="${script_dir}/run_orangepi.sh"
service_name="${ORANGEPI_MEMORY_SERVICE_NAME:-orangepi-rag.service}"
service_user="${ORANGEPI_MEMORY_SERVICE_USER:-${SUDO_USER:-orangepi}}"
service_file="/etc/systemd/system/${service_name}"

chmod +x "${run_script}"

echo "准备香橙派运行环境，服务用户: ${service_user}"
if [[ "${service_user}" == "root" ]]; then
    "${run_script}" --prepare-only
else
    sudo -u "${service_user}" -H "${run_script}" --prepare-only
fi

cat > "${service_file}" <<SERVICE
[Unit]
Description=OrangePi RAG Memory Service
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
User=${service_user}
WorkingDirectory=${repo_root}
Environment=ORANGEPI_MEMORY_HOST=0.0.0.0
Environment=ORANGEPI_MEMORY_PORT=8765
Environment=ORANGEPI_MEMORY_EMBED_BACKEND=hashing
Environment=PYTHONUNBUFFERED=1
ExecStart=${run_script}
Restart=always
RestartSec=3

[Install]
WantedBy=multi-user.target
SERVICE

systemctl daemon-reload
systemctl enable --now "${service_name}"

echo "systemd 服务已启动: ${service_name}"
echo "查看状态: systemctl status ${service_name}"
echo "查看日志: journalctl -u ${service_name} -f"
