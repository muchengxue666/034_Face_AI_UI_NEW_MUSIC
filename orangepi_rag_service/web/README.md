# 香橙派记忆中枢管理后台

## 1. 这个文件夹是干什么的

这个目录里的内容是香橙派记忆中枢的浏览器页面，管理后台和本地联调测试都集中在 `/admin`。

它解决的是这几个问题：

- 你手上暂时没有开发板，没法走完整的设备链路
- 你想先在电脑上验证记忆导入、RAG 编排、家属留言写入是否正常
- 你需要一个浏览器可视化入口，而不是每次都手敲 HTTP 请求

后台里的联调测试工具不会替代正式服务逻辑，它只是把已经存在的后端接口挂成浏览器操作区，方便本地联调。

## 2. 文件结构

- `admin.html`
  记忆管理后台页面，内置联调测试工具
- `routes.py`
  管理后台、兼容跳转和调试接口定义
- `../run_local.ps1`
  Windows PowerShell 启动脚本，默认用本机地址起服务

## 3. 后台能做什么

目前后台可以直接覆盖下面这些能力：

- 查看服务健康状态 `/api/v1/health`
- 重新导入结构化记忆库 `memory_catalog.yaml`
- 从旧文本 `family_memory.txt` 迁移导入
- 执行 RAG 编排 `/api/v1/rag/compose`
- 手工新增一条记忆 `/api/v1/memories/upsert`
- 搜索、编辑、停用记忆 `/api/v1/memories`
- 查看当前 `memory_catalog.yaml` 的内容
- 快速模拟“家属留言写入后，再次问答能否命中”

换句话说，ESP32 那一侧现在主要还原不了的是：

- 真正的录音、STT、TTS
- ESP32 对香橙派的网络探测和异常退化流程
- 屏幕端 UI、语音播放、硬件中断等嵌入式行为

但“树莓派/香橙派本地记忆服务本身”你已经可以在电脑上单独验证。

## 4. 本地怎么启动

### 方式 A：直接用启动脚本

在仓库根目录执行：

```powershell
cd C:\Users\jinch\OneDrive\Competition\Embedded_Chip\034_Face_AI_UI_NEW_MUSIC
.\orangepi_rag_service\run_local.ps1
```

然后在浏览器打开：

```text
以脚本打印的地址为准，通常是 http://127.0.0.1:8765/admin
```

如果 `8765` 被系统拒绝，脚本会自动换到后续可用端口。也可以手动指定：

```powershell
.\orangepi_rag_service\run_local.ps1 -Port 8766
```

手机访问电脑上的后台时，需要绑定到局域网地址：

```powershell
.\orangepi_rag_service\run_local.ps1 -BindHost 0.0.0.0
```

脚本会打印可用于手机浏览器打开的局域网地址。手机和电脑需要在同一个 Wi-Fi 下，Windows 防火墙也需要允许当前 Python 服务监听。

### 方式 B：手动启动

如果你不想走脚本，也可以手动设置环境变量后启动：

```powershell
cd C:\Users\jinch\OneDrive\Competition\Embedded_Chip\034_Face_AI_UI_NEW_MUSIC
$env:ORANGEPI_MEMORY_HOST = "127.0.0.1"
$env:ORANGEPI_MEMORY_PORT = "8765"
$env:ORANGEPI_MEMORY_EMBED_BACKEND = "hashing"
python -m uvicorn orangepi_rag_service.app:app --host 127.0.0.1 --port 8765 --reload
```

## 5. 启动脚本默认做了什么

`run_local.ps1` 会做下面几件事：

- 切到仓库根目录
- 如果你没有显式指定，就默认尝试绑定到 `127.0.0.1:8765`
- 如果默认端口不可用，就自动换到后续可用端口
- 如果你没有显式指定向量后端，就默认使用 `hashing`
- 用 `uvicorn --reload` 启动后端，改代码后浏览器刷新就能看到效果

这里默认用 `hashing` 的原因很简单：

- 这样本地测试不依赖你电脑上已经下载好中文 embedding 模型
- 没网或者没模型时也能先验证整体业务流程

如果你本机已经准备好了 `sentence-transformers` 和对应模型，可以自行覆盖：

```powershell
$env:ORANGEPI_MEMORY_EMBED_BACKEND = "sentence-transformer"
```

## 6. 浏览器页面里怎么测

### 健康检查

点“刷新健康状态”，可以确认：

- 服务是否正常
- 当前索引版本
- 当前记忆条数
- 当前向量后端模式

### RAG 编排测试

左侧“RAG 编排测试”区域可以直接输入问题，然后调用：

```text
/api/v1/rag/compose
```

建议先试这几类问题：

- `我早上几点吃降压药`
- `李美华是谁`
- `奶奶中午几点午睡`
- `家里人什么时候来看我`

### 家属留言写入测试

在“家属留言/记忆写入测试”区域填内容，点击“写入记忆”，会调用：

```text
/api/v1/memories/upsert
```

你可以这样验证联动：

1. 先写入一条新留言，比如“周末我带孩子来看你”
2. 再去 RAG 编排区提问“家里人什么时候来看我”
3. 看返回的 hits 里是否出现刚写入的 note 记忆

### 查看当前 YAML

点“查看当前 YAML”会走：

```text
/api/v1/debug/catalog
```

它是测试专用接口，用来确认当前服务使用的结构化记忆内容到底是什么。

## 7. 后台和正式服务怎么连接

主服务文件是：

- `orangepi_rag_service/app.py`

这个文件会直接挂载：

- `orangepi_rag_service.web.routes`

管理后台和联调测试现在都属于 `orangepi_rag_service/` 服务本体的一部分。

也就是说：

- 正式逻辑依然在 `orangepi_rag_service/`
- Web 页面集中放在 `orangepi_rag_service/web/`
- `/admin` 和正式 API 由同一个 FastAPI 应用挂载

## 8. 如果以后不需要页面

如果后续不需要浏览器页面，可以从 `orangepi_rag_service/app.py` 里移除这行挂载：

```python
app.include_router(web_router)
```

移除后会发生的变化：

- `/admin` 页面消失
- `/api/v1/debug/catalog` 调试接口消失
- 正式 API 仍然可以正常工作

不会影响这些正式接口：

- `/api/v1/health`
- `/api/v1/rag/compose`
- `/api/v1/memories/upsert`
- `/api/v1/memories`
- `/api/v1/memories/import`

## 9. 常见问题

### 1）页面能打开，但没有数据

先点“刷新健康状态”，确认服务已经成功启动。

如果健康检查正常，但没有记忆内容，优先点：

- “重新导入 memory_catalog.yaml”

### 2）命中效果一般

如果你现在是本地 `hashing` 模式，语义效果本来就只是“流程可测”，不是最终最强效果。
要测更接近真实香橙派表现的检索，建议在本机准备好 embedding 模型后改成：

```powershell
$env:ORANGEPI_MEMORY_EMBED_BACKEND = "sentence-transformer"
```

### 3）我只想测接口，不想要页面

可以，不打开 `/admin`，直接用 Postman、curl 或你自己的前端调这几个正式接口即可。

## 10. 建议的使用顺序

如果你第一次本地测试，建议按这个顺序来：

1. 启动 `run_local.ps1`
2. 打开 `/admin`
3. 点“刷新健康状态”
4. 点“重新导入 YAML”
5. 切到“联调测试”
6. 试一条药物/作息问题，确认编排结果正常
7. 写入一条家属留言
8. 再提一个会命中这条留言的问题，确认新增记忆已经进入检索链路

这样一轮下来，基本就能确认“树莓派/香橙派部分”已经能在电脑上独立测试。
