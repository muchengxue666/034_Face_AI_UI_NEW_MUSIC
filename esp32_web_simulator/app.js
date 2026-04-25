const ORANGEPI_SYSTEM_PROMPT_MAX = 1200;
const ORANGEPI_TRACE_ID_MAX = 64;
const ORANGEPI_TIMEOUT_MS = 3500;
const HEALTH_RESPONSE_CAP = 512;
const COMPOSE_RESPONSE_CAP = 4096;
const UPSERT_RESPONSE_CAP = 768;

const els = {
    baseUrl: document.getElementById("baseUrl"),
    saveBaseUrl: document.getElementById("saveBaseUrl"),
    serviceMetric: document.getElementById("serviceMetric"),
    serviceState: document.getElementById("serviceState"),
    memoryCount: document.getElementById("memoryCount"),
    backendMetric: document.getElementById("backendMetric"),
    backendState: document.getElementById("backendState"),
    indexVersion: document.getElementById("indexVersion"),
    traceId: document.getElementById("traceId"),
    elapsedMs: document.getElementById("elapsedMs"),
    initBtn: document.getElementById("initBtn"),
    resetBtn: document.getElementById("resetBtn"),
    probeBtn: document.getElementById("probeBtn"),
    fullFlowBtn: document.getElementById("fullFlowBtn"),
    strictMode: document.getElementById("strictMode"),
    queryInput: document.getElementById("queryInput"),
    composeBtn: document.getElementById("composeBtn"),
    senderInput: document.getElementById("senderInput"),
    messageInput: document.getElementById("messageInput"),
    upsertBtn: document.getElementById("upsertBtn"),
    recentBtn: document.getElementById("recentBtn"),
    logBox: document.getElementById("logBox"),
    promptTab: document.getElementById("promptTab"),
    jsonTab: document.getElementById("jsonTab"),
    promptBox: document.getElementById("promptBox"),
    jsonBox: document.getElementById("jsonBox"),
    hitsBox: document.getElementById("hitsBox"),
    requestBox: document.getElementById("requestBox"),
    responseBox: document.getElementById("responseBox"),
    recentBox: document.getElementById("recentBox"),
    clearMirrorBtn: document.getElementById("clearMirrorBtn"),
};

const state = {
    serviceAvailable: false,
    initialized: false,
    busy: false,
};

function nowTime() {
    return new Date().toLocaleTimeString("zh-CN", { hour12: false });
}

function log(line, level = "INFO") {
    els.logBox.textContent += `${nowTime()} ${level}: ${line}\n`;
    els.logBox.scrollTop = els.logBox.scrollHeight;
}

function setBusy(busy) {
    state.busy = busy;
    for (const button of document.querySelectorAll("button")) {
        button.disabled = busy;
    }
}

function setServiceAvailable(available, label = null) {
    state.serviceAvailable = available;
    els.serviceMetric.className = `metric ${available ? "ok" : "bad"}`;
    els.serviceState.textContent = label || (available ? "可用" : "不可用");
}

function setBackend(payload) {
    els.backendState.textContent = payload?.embedding_backend || "-";
    els.backendMetric.className = `metric ${payload?.degraded ? "warn" : "ok"}`;
}

function saveBaseUrl() {
    const value = normalizeBaseUrl(els.baseUrl.value);
    els.baseUrl.value = value;
    localStorage.setItem("esp32-sim-orangepi-base", value);
    log(`香橙派地址设置为 ${value}`);
}

function normalizeBaseUrl(value) {
    return value.trim().replace(/\/+$/, "");
}

function proxyUrl(path) {
    return `/proxy${path}`;
}

function renderJson(target, value) {
    target.textContent = JSON.stringify(value, null, 2);
}

function randomNoteId() {
    const value = Math.floor(Math.random() * 0xffffffff).toString(16).padStart(8, "0");
    return `note_${value}`;
}

function utf8ByteLength(text) {
    return new TextEncoder().encode(text).length;
}

function applyEsp32Buffer(text, cap, label) {
    if (!els.strictMode.checked) {
        return text;
    }
    const bytes = new TextEncoder().encode(text);
    if (bytes.length < cap) {
        return text;
    }
    log(`${label} response truncated at ${cap - 1} bytes`, "WARN");
    return new TextDecoder().decode(bytes.slice(0, cap - 1));
}

async function requestOrangePi(path, options = {}, cap = 4096, label = "OrangePi") {
    const base = normalizeBaseUrl(els.baseUrl.value);
    const requestPayload = options.body ? JSON.parse(options.body) : null;
    renderJson(els.requestBox, {
        method: options.method || "GET",
        url: `${base}${path}`,
        timeout_ms: els.strictMode.checked ? ORANGEPI_TIMEOUT_MS : 10000,
        payload: requestPayload,
    });

    const started = performance.now();
    const response = await fetch(proxyUrl(path), {
        method: options.method || "GET",
        headers: {
            "Content-Type": "application/json",
            "X-OrangePi-Base": base,
            "X-OrangePi-Timeout-Ms": String(els.strictMode.checked ? ORANGEPI_TIMEOUT_MS : 10000),
        },
        body: options.body,
    });
    const raw = await response.text();
    const limited = applyEsp32Buffer(raw, cap, label);
    const cost = Math.round(performance.now() - started);
    let payload = null;
    try {
        payload = limited ? JSON.parse(limited) : null;
    } catch (error) {
        renderJson(els.responseBox, {
            http_status: response.status,
            parse_error: String(error),
            raw: limited,
            raw_bytes: utf8ByteLength(raw),
            elapsed_ms: cost,
        });
        throw new Error(`${label} response JSON parse failed`);
    }

    renderJson(els.responseBox, {
        http_status: response.status,
        raw_bytes: utf8ByteLength(raw),
        elapsed_ms: cost,
        payload,
    });
    if (!response.ok) {
        throw new Error(payload?.detail || `${label} failed, http=${response.status}`);
    }
    return payload;
}

function initClient() {
    state.initialized = true;
    setServiceAvailable(false, "已初始化");
    els.memoryCount.textContent = "-";
    els.backendState.textContent = "-";
    els.indexVersion.textContent = "-";
    els.traceId.textContent = "-";
    els.elapsedMs.textContent = "-";
    log("orangepi_memory_client_init()");
}

async function probeService() {
    if (!state.initialized) {
        initClient();
    }
    try {
        log("GET /api/v1/health");
        const payload = await requestOrangePi("/api/v1/health", {}, HEALTH_RESPONSE_CAP, "Health");
        setServiceAvailable(true, "可用");
        els.memoryCount.textContent = String(payload.memory_count ?? "-");
        els.indexVersion.textContent = payload.index_version || "-";
        setBackend(payload);
        log("Health check: OK (http=200)");
        return true;
    } catch (error) {
        setServiceAvailable(false, "不可用");
        log(`Health check: FAIL (${error.message})`, "WARN");
        return false;
    }
}

async function composePrompt() {
    const query = els.queryInput.value.trim();
    if (!query) {
        log("Compose skipped: query is empty", "WARN");
        return null;
    }
    if (!state.serviceAvailable) {
        log("Compose skipped: service unavailable", "WARN");
        return null;
    }

    const body = {
        query,
        session_mode: "elder_companion",
        max_prompt_chars: ORANGEPI_SYSTEM_PROMPT_MAX,
    };
    try {
        log("POST /api/v1/rag/compose");
        const payload = await requestOrangePi(
            "/api/v1/rag/compose",
            { method: "POST", body: JSON.stringify(body) },
            COMPOSE_RESPONSE_CAP,
            "Compose",
        );
        if (!payload.system_prompt) {
            setServiceAvailable(false, "不可用");
            throw new Error("Compose response missing system_prompt");
        }
        const prompt = payload.system_prompt.slice(0, ORANGEPI_SYSTEM_PROMPT_MAX);
        els.promptBox.textContent = prompt;
        renderJson(els.jsonBox, payload);
        els.traceId.textContent = String(payload.trace_id || "-").slice(0, ORANGEPI_TRACE_ID_MAX - 1);
        els.elapsedMs.textContent = `${payload.elapsed_ms ?? "-"} ms`;
        renderHits(payload.hits || []);
        setServiceAvailable(true, "可用");
        log(`OrangePi prompt ready: trace=${payload.trace_id}, has_memory=${payload.has_memory}, degraded=${payload.degraded}, elapsed=${payload.elapsed_ms}ms`);
        return payload;
    } catch (error) {
        setServiceAvailable(false, "不可用");
        log(`Compose failed: ${error.message}`, "WARN");
        return null;
    }
}

async function upsertNote() {
    const sender = els.senderInput.value.trim();
    const message = els.messageInput.value.trim();
    if (!sender || !message) {
        log("Upsert skipped: sender or message is empty", "WARN");
        return null;
    }

    const content = `发送人：${sender}\n内容：${message}`;
    const body = {
        items: [
            {
                id: randomNoteId(),
                category: "note",
                title: "家属留言",
                content,
                source: "mqtt_app",
                priority: 70,
                keywords: [sender, "家属留言", "来看你"],
            },
        ],
    };

    try {
        log("POST /api/v1/memories/upsert");
        const payload = await requestOrangePi(
            "/api/v1/memories/upsert",
            { method: "POST", body: JSON.stringify(body) },
            UPSERT_RESPONSE_CAP,
            "Upsert",
        );
        setServiceAvailable(true, "可用");
        els.indexVersion.textContent = payload.index_version || "-";
        log("Note synced to OrangePi memory service");
        await loadRecentNotes();
        return payload;
    } catch (error) {
        log(`Upsert note failed: ${error.message}`, "WARN");
        return null;
    }
}

async function loadRecentNotes() {
    try {
        const payload = await requestOrangePi("/api/v1/memories/recent?category=note&limit=8", {}, HEALTH_RESPONSE_CAP * 8, "Recent");
        renderRecent(payload.items || []);
        return payload;
    } catch (error) {
        log(`Recent notes failed: ${error.message}`, "WARN");
        return null;
    }
}

function renderHits(hits) {
    if (!hits.length) {
        els.hitsBox.innerHTML = `<div class="empty">没有命中记忆</div>`;
        return;
    }
    els.hitsBox.innerHTML = hits.map((hit) => `
        <div class="hit">
            <strong>${escapeHtml(hit.title || hit.id)}</strong>
            <small>${escapeHtml(hit.category)} / ${escapeHtml(hit.source)} / score=${escapeHtml(String(hit.score))}</small>
            <div>${escapeHtml(hit.summary || "")}</div>
        </div>
    `).join("");
}

function renderRecent(items) {
    if (!items.length) {
        els.recentBox.innerHTML = `<div class="empty">没有最近留言</div>`;
        return;
    }
    els.recentBox.innerHTML = items.map((item) => `
        <div class="hit">
            <strong>${escapeHtml(item.title || item.id)}</strong>
            <small>${escapeHtml(item.source)} / ${escapeHtml(item.updated_at || "")}</small>
            <div>${escapeHtml(item.content || "")}</div>
        </div>
    `).join("");
}

function escapeHtml(value) {
    return value
        .replaceAll("&", "&amp;")
        .replaceAll("<", "&lt;")
        .replaceAll(">", "&gt;")
        .replaceAll('"', "&quot;")
        .replaceAll("'", "&#039;");
}

async function fullFlow() {
    const ok = state.serviceAvailable || await probeService();
    if (!ok) {
        log("Pipeline fallback to default assistant prompt", "WARN");
        return;
    }
    const result = await composePrompt();
    if (!result) {
        log("Pipeline fallback to default assistant prompt", "WARN");
    }
}

function resetClient() {
    state.initialized = false;
    setServiceAvailable(false, "未初始化");
    els.memoryCount.textContent = "-";
    els.backendState.textContent = "-";
    els.indexVersion.textContent = "-";
    els.traceId.textContent = "-";
    els.elapsedMs.textContent = "-";
    els.promptBox.textContent = "";
    els.jsonBox.textContent = "";
    els.hitsBox.innerHTML = `<div class="empty">等待 RAG 结果</div>`;
    els.recentBox.innerHTML = `<div class="empty">等待刷新</div>`;
    els.requestBox.textContent = "";
    els.responseBox.textContent = "";
    els.logBox.textContent = "";
    log("ESP32 simulator reset");
}

function bindEvents() {
    els.saveBaseUrl.addEventListener("click", saveBaseUrl);
    els.initBtn.addEventListener("click", initClient);
    els.resetBtn.addEventListener("click", resetClient);
    els.probeBtn.addEventListener("click", () => runAction(probeService));
    els.composeBtn.addEventListener("click", () => runAction(composePrompt));
    els.upsertBtn.addEventListener("click", () => runAction(upsertNote));
    els.recentBtn.addEventListener("click", () => runAction(loadRecentNotes));
    els.fullFlowBtn.addEventListener("click", () => runAction(fullFlow));
    els.clearMirrorBtn.addEventListener("click", () => {
        els.requestBox.textContent = "";
        els.responseBox.textContent = "";
    });
    els.promptTab.addEventListener("click", () => switchTab("prompt"));
    els.jsonTab.addEventListener("click", () => switchTab("json"));
    for (const button of document.querySelectorAll("[data-query]")) {
        button.addEventListener("click", () => {
            els.queryInput.value = button.dataset.query;
        });
    }
}

async function runAction(action) {
    if (state.busy) {
        return;
    }
    setBusy(true);
    try {
        await action();
    } finally {
        setBusy(false);
    }
}

function switchTab(name) {
    const showPrompt = name === "prompt";
    els.promptTab.classList.toggle("active", showPrompt);
    els.jsonTab.classList.toggle("active", !showPrompt);
    els.promptBox.hidden = !showPrompt;
    els.jsonBox.hidden = showPrompt;
}

function boot() {
    const saved = localStorage.getItem("esp32-sim-orangepi-base");
    if (saved) {
        els.baseUrl.value = saved;
    }
    bindEvents();
    resetClient();
}

boot();
