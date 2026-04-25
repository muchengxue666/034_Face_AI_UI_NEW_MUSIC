# ESP32 网页模拟器

这个文件夹用于在没有 ESP32 开发板时，从电脑网页模拟 `orangepi_memory_client.c` 对香橙派记忆服务的访问。

## 功能

- 探测 `/api/v1/health`
- 调用 `/api/v1/rag/compose`
- 按 ESP32 payload 写入 `/api/v1/memories/upsert`
- 查看最近家属留言
- 模拟 ESP32 的服务可用标记、3500ms 超时、响应缓冲区截断

## 启动

在项目根目录执行：

```powershell
python .\esp32_web_simulator\server.py --host 127.0.0.1 --port 8780
```

打开：

```text
http://127.0.0.1:8780
```

默认香橙派地址是：

```text
http://192.168.1.190:8765
```

也可以启动前覆盖：

```powershell
$env:ORANGEPI_MEMORY_BASE_URL = "http://你的香橙派IP:8765"
python .\esp32_web_simulator\server.py
```

网页里也可以直接修改香橙派服务地址。

## 和 ESP32 代码的对应关系

- 探测逻辑对应 `orangepi_memory_client_probe`
- RAG 编排对应 `orangepi_memory_client_compose_prompt`
- 家属留言写入对应 `orangepi_memory_client_upsert_note`
- 严格模式使用：
  - `ORANGEPI_TIMEOUT_MS = 3500`
  - health 响应缓冲区 `512`
  - compose 响应缓冲区 `4096`
  - upsert 响应缓冲区 `768`

如果 `/api/v1/health` 正常，但网页请求失败，优先确认电脑能访问：

```powershell
curl http://192.168.1.190:8765/api/v1/health
```
