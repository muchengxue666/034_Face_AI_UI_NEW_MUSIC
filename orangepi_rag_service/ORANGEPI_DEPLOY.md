# 香橙派部署说明

这个服务现在按“香橙派常驻运行，ESP32 和电脑远程访问”的方式部署。

## 1. 部署目标

- 香橙派监听 `0.0.0.0:8765`
- ESP32 通过 HTTP 调用 `/api/v1/rag/compose` 和 `/api/v1/memories/upsert`
- 电脑可以像 ESP32 一样直接访问香橙派 API
- 默认使用 `hashing` 向量后端，不依赖 `sentence-transformers`

## 2. 首次安装

把项目放到香橙派，例如：

```bash
cd /home/orangepi
git clone <你的仓库地址> 034_Face_AI_UI_NEW_MUSIC
cd 034_Face_AI_UI_NEW_MUSIC
```

安装系统依赖：

```bash
sudo apt update
sudo apt install -y python3 python3-venv python3-pip
```

前台试运行：

```bash
bash orangepi_rag_service/run_orangepi.sh
```

看到服务启动后，在电脑上访问：

```bash
curl http://香橙派IP:8765/api/v1/health
```

## 3. 开机自启

在项目根目录执行：

```bash
sudo bash orangepi_rag_service/install_orangepi_service.sh
```

查看服务状态：

```bash
systemctl status orangepi-rag.service
```

查看实时日志：

```bash
journalctl -u orangepi-rag.service -f
```

如果系统开启了防火墙，放行端口：

```bash
sudo ufw allow 8765/tcp
```

## 4. 电脑远程测试

健康检查：

```bash
curl http://香橙派IP:8765/api/v1/health
```

RAG 编排：

```bash
curl -X POST http://香橙派IP:8765/api/v1/rag/compose \
  -H "Content-Type: application/json" \
  -d '{"query":"我早上几点吃降压药","session_mode":"elder_companion","max_prompt_chars":1200}'
```

写入一条家属留言：

```bash
curl -X POST http://香橙派IP:8765/api/v1/memories/upsert \
  -H "Content-Type: application/json" \
  -d '{"items":[{"category":"note","title":"家属留言","content":"发送人：女儿\n内容：周末我带孩子来看你。","keywords":["女儿","家属留言","来看你"],"source":"mqtt_app","priority":70}]}'
```

## 5. ESP32 配置

ESP32 端需要把香橙派 IP 改成板子的固定局域网地址：

```c
#define ORANGEPI_SERVICE_IP "香橙派IP"
```

对应文件：

```text
main/APP/ORANGEPI/orangepi_memory_client.c
```

建议在路由器里给香橙派绑定固定 IP，避免每次重启后 ESP32 找不到服务。

## 6. 可选语义模型

默认 `hashing` 后端适合稳定部署。如果后续要试 `sentence-transformer`：

```bash
export ORANGEPI_MEMORY_EMBED_BACKEND=sentence-transformer
```

注意当前代码使用 CPU 跑 `sentence-transformers`，不会自动调用 AIpro 8T 的 NPU；模型也需要提前放到本地缓存。
