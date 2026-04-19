/**
 ******************************************************************************
 * @file        voice_pipeline.c
 * @version     V1.0
 * @brief       语音交互流水线实现
 *
 *  流程：
 *    voice_pipeline_trigger()
 *        │
 *        ▼
 *    录音（I2S采集，最长10秒，静音自动停止）
 *        │
 *        ▼
 *    STT（火山引擎 ASR HTTP API → 返回文本）
 *        │
 *        ▼
 *    LLM（豆包大模型 → 返回回复文本）
 *        │
 *        ▼
 *    TTS（火山引擎 TTS HTTP API → 返回PCM音频）
 *        │
 *        ▼
 *    ES8311 播放（I2S输出）
 *
 ******************************************************************************
 */

#include "voice_pipeline.h"
#include "avatar_screen.h"
#include "mipi_cam.h"
#include "myi2s.h"
#include "myes8311.h"
#include "inmp441.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_codec_dev.h"
#include "cJSON.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

static const char *TAG = "VOICE";

/* ==================== 火山引擎 API 配置 ==================== */
#define VOLCENGINE_API_KEY      "89c5ffe9-8cba-466a-9007-f9ebc4817b7f"
#define DOUBAO_ENDPOINT         "ep-20260310180907-frhfb"

/* ==================== 香橙派 RAG 服务配置 ==================== */
/* 修改为香橙派 AIPro 的局域网 IP 地址 */
#define RAG_SERVICE_IP          "192.168.43.26"
#define RAG_SERVICE_PORT        8765
#define RAG_URL                 "http://" RAG_SERVICE_IP ":" "8765" "/query"
#define RAG_HEALTH_URL          "http://" RAG_SERVICE_IP ":" "8765" "/health"
#define RAG_TIMEOUT_MS          3000    /* RAG 查询超时：3秒，超时则退化为无记忆模式 */
#define RAG_CONTEXT_MAX         800     /* 注入 system prompt 的最大记忆字符数 */

/* 火山引擎 ASR（语音识别）HTTP接口 */
#define ASR_V1_URL              "https://openspeech.bytedance.com/api/v1/asr"
#define ASR_V3_URL              "https://openspeech.bytedance.com/api/v3/auc/bigmodel/recognize/flash"
#define ASR_V3_RESOURCE_ID      "volc.bigasr.auc_turbo"
#define ASR_URL                 ASR_V1_URL
#define ASR_CLUSTER             "volcengine_input_common"
#define ASR_WORKFLOW            "audio_in,resample,partition,vad,fe,decode,itn,nlu_punctuate"
#define ASR_APP_ID              "5115099307"
#define ASR_ACCESS_TOKEN        "82YMkqc5qP1x2i05mFeYFJm8dkvaJuYM"
#define ASR_LANGUAGE            "zh-CN"
#define ASR_USE_WAV_UPLOAD      1
#define ASR_START_SILENCE_MS    "5000"
#define ASR_VAD_SILENCE_MS      "800"

/* 火山引擎 TTS（语音合成）HTTP接口 */
#define TTS_URL                 "https://openspeech.bytedance.com/api/v1/tts"
#define TTS_APP_ID              "8885749333"   /* 填入您的AppID */
#define TTS_ACCESS_TOKEN        "zU5YdnsfQEG9mGgDPQ7DwtFnDYoOBwL2"   /* 填入您的Access Token */
#define TTS_VOICE_TYPE          "zh_female_qingxin"  /* 温和女声音色 */

/* ==================== 录音配置 ==================== */
#define RECORD_SAMPLE_RATE      16000               /* 16kHz 采样率 */
#define RECORD_MAX_SECONDS      10                  /* 最长录音时间 */
#define RECORD_BUF_SIZE         (RECORD_SAMPLE_RATE * 2 * RECORD_MAX_SECONDS)  /* 最大录音缓冲（16bit单声道）*/
#define SILENCE_THRESHOLD       350                 /* 静音检测阈值（RMS能量）— 信号放大后底噪RMS约200~250 */
#define SPEECH_THRESHOLD        900                 /* 判断“开始说话”的 RMS 阈值 */
#define END_SILENCE_THRESHOLD   600                 /* 说话结束后，低于此值按静音计时 */
#define END_SILENCE_DURATION_MS 800                 /* 说完后 0.8 秒没再说就停录 */
#define MIN_SPEECH_MS           320                 /* 至少有一小段有效语音再允许停录 */
#define CHUNK_SAMPLES           1024                /* 每次读取的采样点数 */
#define CHUNK_BYTES             (CHUNK_SAMPLES * 2) /* 每次读取字节数（16bit）*/

/* ==================== 内部状态 ==================== */
/* Upload-time trim and gain guardrails */
#define TRIM_MIN_THRESHOLD      40                  /* 降低裁剪下限，避免把弱语音开头切掉 */
#define TRIM_MAX_THRESHOLD      400                 /* 降低裁剪上限 */
#define TRIM_KEEP_SAMPLES       4800                /* 裁剪后保留前后约300ms，给更多余量 */
#define NORMALIZE_TARGET_PEAK   28000               /* 提高目标峰值，让人声更饱满 */
#define NORMALIZE_MAX_GAIN      12.0f               /* 允许更大增益（信号本身弱就需要放大） */
#define NORMALIZE_MIN_GAIN      1.0f                /* 任何增益都做（哪怕只放大一点点） */
#define AUDIO_LOW_RMS_WARN      800

static volatile voice_state_t s_state = VOICE_STATE_IDLE;
static voice_state_cb_t       s_state_cb = NULL;
static SemaphoreHandle_t      s_trigger_sem = NULL;
static SemaphoreHandle_t      s_runtime_mutex = NULL;
static TaskHandle_t           s_pipeline_task = NULL;
static bool                   s_voice_ready = false;

/* 录音缓冲（PSRAM分配）*/
static uint8_t *s_record_buf  = NULL;
static uint32_t s_record_len  = 0;

/* HTTP 回调接收缓冲（STT/TTS 用）*/
#define HTTP_RECV_BUF_SIZE      (512 * 1024)
static uint8_t *s_http_buf    = NULL;
static uint32_t s_http_len    = 0;
static bool s_http_overflow   = false;

/* RAG 专用接收缓冲（与 STT/TTS 分开，避免互相覆盖）*/
#define RAG_RECV_BUF_SIZE       (4 * 1024)
static uint8_t *s_rag_buf     = NULL;
static uint32_t s_rag_len     = 0;

/* RAG 服务是否可用（启动时探测，运行时动态更新）*/
static volatile bool s_rag_available = false;

/* ==================== 工具函数 ==================== */

static void set_state(voice_state_t state)
{
    s_state = state;
    if (s_state_cb) {
        s_state_cb(state);
    }
}

static bool runtime_lock(TickType_t ticks_to_wait)
{
    return s_runtime_mutex && xSemaphoreTake(s_runtime_mutex, ticks_to_wait) == pdTRUE;
}

static void runtime_unlock(void)
{
    if (s_runtime_mutex) {
        xSemaphoreGive(s_runtime_mutex);
    }
}

static void voice_camera_pause(bool paused)
{
    if (s_voice_ready) {
        mipi_cam_set_paused(paused);
    }
}

/**
 * @brief 计算 PCM 音频片段的 RMS 能量（用于静音检测）
 */
static uint32_t calc_rms(const int16_t *samples, int count)
{
    int64_t sum = 0;
    for (int i = 0; i < count; i++) {
        sum += (int64_t)samples[i] * samples[i];
    }
    return (uint32_t)sqrtf((float)(sum / count));
}

static void play_stereo_block(const int16_t *stereo_samples, size_t stereo_sample_count)
{
    if (!stereo_samples || stereo_sample_count == 0) return;
    i2s_tx_write((uint8_t *)stereo_samples, stereo_sample_count * sizeof(int16_t));
}

static esp_err_t play_tone_segment(float freq_hz, uint32_t duration_ms, float amplitude)
{
    const uint32_t sample_rate = RECORD_SAMPLE_RATE;
    const uint32_t total_samples = (sample_rate * duration_ms) / 1000;
    const uint32_t block_samples = 192;
    int16_t stereo_block[block_samples * 2];

    i2s_set_samplerate_bits_sample(sample_rate, I2S_DATA_BIT_WIDTH_16BIT);

    for (uint32_t offset = 0; offset < total_samples; offset += block_samples) {
        uint32_t count = ((total_samples - offset) < block_samples) ? (total_samples - offset) : block_samples;
        for (uint32_t i = 0; i < count; i++) {
            float t = (float)(offset + i) / (float)sample_rate;
            float fade = 1.0f - ((float)(offset + i) / (float)(total_samples ? total_samples : 1)) * 0.25f;
            int16_t v = (int16_t)(sinf(2.0f * (float)M_PI * freq_hz * t) * 32767.0f * amplitude * fade);
            stereo_block[i * 2] = v;
            stereo_block[i * 2 + 1] = v;
        }
        play_stereo_block(stereo_block, count * 2);
    }

    return ESP_OK;
}

static esp_err_t play_silence_segment(uint32_t duration_ms)
{
    const uint32_t sample_rate = RECORD_SAMPLE_RATE;
    const uint32_t total_samples = (sample_rate * duration_ms) / 1000;
    const uint32_t block_samples = 192;
    int16_t stereo_block[192 * 2];
    memset(stereo_block, 0, sizeof(stereo_block));

    i2s_set_samplerate_bits_sample(sample_rate, I2S_DATA_BIT_WIDTH_16BIT);

    for (uint32_t offset = 0; offset < total_samples; offset += block_samples) {
        uint32_t count = ((total_samples - offset) < block_samples) ? (total_samples - offset) : block_samples;
        play_stereo_block(stereo_block, count * 2);
    }

    return ESP_OK;
}

static esp_err_t play_notification_chime_locked(void)
{
    ESP_LOGI(TAG, "Playing notification chime");
    play_tone_segment(784.0f, 110, 0.18f);
    play_silence_segment(35);
    play_tone_segment(1046.5f, 130, 0.16f);
    play_silence_segment(35);
    play_tone_segment(1318.5f, 220, 0.14f);
    return ESP_OK;
}

static uint32_t preprocess_recorded_audio(uint8_t *buf, uint32_t len)
{
    if (!buf || len < 2) return 0;

    int16_t *samples = (int16_t *)buf;
    uint32_t num_samples = len / 2;
    uint32_t original_samples = num_samples;
    int64_t sum = 0;
    int32_t peak = 0;

    for (uint32_t i = 0; i < num_samples; i++) {
        sum += samples[i];
        int32_t abs_v = samples[i] >= 0 ? samples[i] : -samples[i];
        if (abs_v > peak) peak = abs_v;
    }

    int16_t dc_offset = (int16_t)(sum / (int64_t)num_samples);
    for (uint32_t i = 0; i < num_samples; i++) {
        samples[i] -= dc_offset;
    }
    ESP_LOGI(TAG, "DC offset removed: %d", dc_offset);

    int32_t trim_threshold = peak / 40;  /* 更保守的裁剪阈值（之前 /24 太激进） */
    if (trim_threshold < TRIM_MIN_THRESHOLD) trim_threshold = TRIM_MIN_THRESHOLD;
    if (trim_threshold > TRIM_MAX_THRESHOLD) trim_threshold = TRIM_MAX_THRESHOLD;

    uint32_t start = 0;
    while (start < num_samples) {
        int32_t abs_v = samples[start] >= 0 ? samples[start] : -samples[start];
        if (abs_v >= trim_threshold) break;
        start++;
    }

    uint32_t end = num_samples;
    while (end > start) {
        int32_t abs_v = samples[end - 1] >= 0 ? samples[end - 1] : -samples[end - 1];
        if (abs_v >= trim_threshold) break;
        end--;
    }

    if (start > TRIM_KEEP_SAMPLES) {
        start -= TRIM_KEEP_SAMPLES;
    } else {
        start = 0;
    }
    if (end + TRIM_KEEP_SAMPLES < num_samples) {
        end += TRIM_KEEP_SAMPLES;
    } else {
        end = num_samples;
    }

    if (start > 0 || end < num_samples) {
        memmove(samples, samples + start, (end - start) * sizeof(int16_t));
        num_samples = end - start;
        len = num_samples * 2;
        ESP_LOGI(TAG,
                 "Audio trimmed: threshold=%ld, leading=%lu samples, trailing=%lu samples, kept=%lu bytes",
                 (long)trim_threshold, start, original_samples - end, len);
    }

    if (num_samples == 0) {
        ESP_LOGW(TAG, "Audio preprocessing removed all samples");
        return 0;
    }

    peak = 0;
    for (uint32_t i = 0; i < num_samples; i++) {
        int32_t abs_v = samples[i] >= 0 ? samples[i] : -samples[i];
        if (abs_v > peak) peak = abs_v;
    }
    ESP_LOGI(TAG, "Audio stats: peak=%ld, trim_threshold=%ld", (long)peak, (long)trim_threshold);

    if (peak > 100 && peak < NORMALIZE_TARGET_PEAK) {
        float gain = (float)NORMALIZE_TARGET_PEAK / (float)peak;
        if (gain > NORMALIZE_MAX_GAIN) gain = NORMALIZE_MAX_GAIN;
        if (gain >= NORMALIZE_MIN_GAIN) {
            for (uint32_t i = 0; i < num_samples; i++) {
                int32_t v = (int32_t)lrintf(samples[i] * gain);
                if (v > 32767) v = 32767;
                if (v < -32768) v = -32768;
                samples[i] = (int16_t)v;
            }
            ESP_LOGI(TAG, "Audio normalized: peak=%ld, gain=%.1fx", (long)peak, gain);
        }
    }

    uint32_t final_rms = calc_rms(samples, num_samples);
    ESP_LOGI(TAG, "Recorded %lu bytes (%.1f seconds), final RMS=%lu",
             len,
             (float)len / (RECORD_SAMPLE_RATE * 2),
             final_rms);
    if (final_rms < AUDIO_LOW_RMS_WARN) {
        ESP_LOGW(TAG, "Audio RMS is still low after preprocessing (%lu)", final_rms);
    }
    return len;
}

static uint8_t *build_wav_from_pcm(const uint8_t *pcm_data, uint32_t pcm_len, uint32_t *out_len)
{
    if (!pcm_data || pcm_len == 0 || !out_len) return NULL;

    uint32_t wav_len = pcm_len + 44;
    uint8_t *wav = malloc(wav_len);
    if (!wav) return NULL;

    uint32_t byte_rate = RECORD_SAMPLE_RATE * 2;
    uint16_t block_align = 2;
    uint32_t riff_chunk_size = wav_len - 8;
    uint32_t data_chunk_size = pcm_len;

    memcpy(wav + 0, "RIFF", 4);
    memcpy(wav + 4, &riff_chunk_size, 4);
    memcpy(wav + 8, "WAVE", 4);
    memcpy(wav + 12, "fmt ", 4);

    uint32_t fmt_chunk_size = 16;
    uint16_t audio_format = 1;
    uint16_t channels = 1;
    uint16_t bits_per_sample = 16;

    memcpy(wav + 16, &fmt_chunk_size, 4);
    memcpy(wav + 20, &audio_format, 2);
    memcpy(wav + 22, &channels, 2);
    memcpy(wav + 24, &(uint32_t){RECORD_SAMPLE_RATE}, 4);
    memcpy(wav + 28, &byte_rate, 4);
    memcpy(wav + 32, &block_align, 2);
    memcpy(wav + 34, &bits_per_sample, 2);
    memcpy(wav + 36, "data", 4);
    memcpy(wav + 40, &data_chunk_size, 4);
    memcpy(wav + 44, pcm_data, pcm_len);

    *out_len = wav_len;
    return wav;
}

/* ==================== HTTP 回调 ==================== */

static int json_extract_response_code(cJSON *root)
{
    if (!root) return 0;

    cJSON *code_obj = cJSON_GetObjectItem(root, "code");
    if (code_obj && cJSON_IsNumber(code_obj)) {
        return code_obj->valueint;
    }

    cJSON *header = cJSON_GetObjectItem(root, "header");
    if (header) {
        code_obj = cJSON_GetObjectItem(header, "code");
        if (code_obj && cJSON_IsNumber(code_obj)) {
            return code_obj->valueint;
        }
    }

    return 0;
}

static const char *json_extract_response_message(cJSON *root)
{
    if (!root) return NULL;

    cJSON *msg_obj = cJSON_GetObjectItem(root, "message");
    if (!msg_obj) msg_obj = cJSON_GetObjectItem(root, "msg");
    if (msg_obj && cJSON_IsString(msg_obj) && msg_obj->valuestring) {
        return msg_obj->valuestring;
    }

    cJSON *header = cJSON_GetObjectItem(root, "header");
    if (header) {
        msg_obj = cJSON_GetObjectItem(header, "message");
        if (!msg_obj) msg_obj = cJSON_GetObjectItem(header, "msg");
        if (msg_obj && cJSON_IsString(msg_obj) && msg_obj->valuestring) {
            return msg_obj->valuestring;
        }
    }

    return NULL;
}

static esp_err_t http_event_collect(esp_http_client_event_t *evt)
{
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        if (s_http_buf) {
            size_t space_left = (HTTP_RECV_BUF_SIZE - 1 > s_http_len) ? (HTTP_RECV_BUF_SIZE - 1 - s_http_len) : 0;
            size_t to_copy = (evt->data_len <= space_left) ? evt->data_len : space_left;
            if (to_copy > 0) {
                memcpy(s_http_buf + s_http_len, evt->data, to_copy);
                s_http_len += to_copy;
            }
            if (to_copy < evt->data_len) {
                s_http_overflow = true;
            }
        }
    }
    return ESP_OK;
}

/* RAG 专用回调，写入独立缓冲，不干扰 STT/TTS */
static esp_err_t http_event_collect_rag(esp_http_client_event_t *evt)
{
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        if (s_rag_buf && (s_rag_len + evt->data_len < RAG_RECV_BUF_SIZE - 1)) {
            memcpy(s_rag_buf + s_rag_len, evt->data, evt->data_len);
            s_rag_len += evt->data_len;
        }
    }
    return ESP_OK;
}

/* ==================== RAG 模块 ==================== */

/**
 * @brief 探测香橙派 RAG 服务是否可达
 * @return true=可用
 */
static bool rag_health_check(void)
{
    esp_http_client_config_t cfg = {
        .url         = RAG_HEALTH_URL,
        .timeout_ms  = RAG_TIMEOUT_MS,
        .event_handler = http_event_collect_rag,
    };
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    esp_http_client_set_method(client, HTTP_METHOD_GET);

    s_rag_len = 0;
    if (s_rag_buf) memset(s_rag_buf, 0, RAG_RECV_BUF_SIZE);

    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    bool ok = (err == ESP_OK && status == 200);
    ESP_LOGI(TAG, "RAG health check: %s (http=%d)", ok ? "OK" : "FAIL", status);
    return ok;
}

/**
 * @brief 查询香橙派 RAG 服务，获取与问题相关的家庭记忆片段
 * @param question   用户问题（STT 识别文本）
 * @param out_ctx    输出记忆上下文缓冲
 * @param out_size   缓冲大小
 * @return true=查询成功且有相关记忆，false=查询失败或无相关记忆
 */
static bool rag_query(const char *question, char *out_ctx, size_t out_size)
{
    if (!s_rag_available || !question || question[0] == '\0') return false;

    s_rag_len = 0;
    if (s_rag_buf) memset(s_rag_buf, 0, RAG_RECV_BUF_SIZE);

    /* 构建请求体：{"question": "...", "top_k": 3} */
    char post_body[512];
    /* 对 question 中的双引号转义，防止 JSON 注入 */
    char safe_q[256];
    size_t qi = 0, si = 0;
    while (question[qi] && si < sizeof(safe_q) - 2) {
        if (question[qi] == '"' || question[qi] == '\\') safe_q[si++] = '\\';
        safe_q[si++] = question[qi++];
    }
    safe_q[si] = '\0';

    snprintf(post_body, sizeof(post_body),
             "{\"question\":\"%s\",\"top_k\":3}", safe_q);

    esp_http_client_config_t cfg = {
        .url           = RAG_URL,
        .timeout_ms    = RAG_TIMEOUT_MS,
        .event_handler = http_event_collect_rag,
        .buffer_size   = 2048,
    };
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_body, strlen(post_body));

    esp_err_t err = esp_http_client_perform(client);
    int status    = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK || status != 200 || s_rag_len == 0) {
        /* 网络抖动或服务重启，标记不可用，下次触发再重试 health check */
        ESP_LOGW(TAG, "RAG query failed (err=%s, http=%d), disabling", esp_err_to_name(err), status);
        s_rag_available = false;
        return false;
    }

    s_rag_buf[s_rag_len] = '\0';

    /* 解析响应：{"context": "...", "sources": [...], "elapsed_ms": 42} */
    cJSON *root = cJSON_Parse((char *)s_rag_buf);
    if (!root) {
        ESP_LOGE(TAG, "RAG: JSON parse failed");
        return false;
    }

    bool success = false;
    cJSON *ctx_obj = cJSON_GetObjectItem(root, "context");
    if (ctx_obj && cJSON_IsString(ctx_obj) && ctx_obj->valuestring[0] != '\0') {
        strncpy(out_ctx, ctx_obj->valuestring, out_size - 1);
        out_ctx[out_size - 1] = '\0';

        cJSON *elapsed = cJSON_GetObjectItem(root, "elapsed_ms");
        ESP_LOGI(TAG, "RAG: got %zu chars of context in %dms",
                 strlen(out_ctx),
                 elapsed ? elapsed->valueint : -1);
        success = true;
    } else {
        /* context 为空字符串：问题与记忆无关，正常情况 */
        ESP_LOGI(TAG, "RAG: no relevant memory for this question");
    }

    cJSON_Delete(root);
    return success;
}

/* ==================== STT 模块 ==================== */

/**
 * @brief 将 PCM 数据发送到火山引擎 ASR 接口，返回识别文本
 * @note  使用 multipart/form-data 上传 PCM 数据
 * @param pcm_data  PCM 数据指针（16bit, 16kHz, mono）
 * @param pcm_len   字节数
 * @param out_text  输出文本缓冲
 * @param out_size  输出缓冲大小
 * @return true=识别成功
 */
static bool stt_recognize(const uint8_t *pcm_data, uint32_t pcm_len,
                           char *out_text, size_t out_size)
{
    if (!pcm_data || pcm_len == 0 || !out_text || out_size == 0) return false;

    const uint8_t *upload_data = pcm_data;
    uint32_t upload_len = pcm_len;
    const char *audio_format = "raw";
    uint8_t *wav_buf = NULL;

#if ASR_USE_WAV_UPLOAD
    uint32_t wav_len = 0;
    wav_buf = build_wav_from_pcm(pcm_data, pcm_len, &wav_len);
    if (wav_buf) {
        upload_data = wav_buf;
        upload_len = wav_len;
        audio_format = "wav";
        ESP_LOGI(TAG, "STT: wrapped PCM into WAV (%lu -> %lu bytes)",
                 (unsigned long)pcm_len, (unsigned long)wav_len);
    } else {
        ESP_LOGW(TAG, "STT: failed to build WAV, falling back to raw PCM upload");
    }
#endif

    ESP_LOGI(TAG, "STT: sending %lu bytes of %s audio...", (unsigned long)upload_len, audio_format);
    ESP_LOGI(TAG, "STT: request url=%s cluster=%s workflow=%s language=%s",
             ASR_URL, ASR_CLUSTER, ASR_WORKFLOW, ASR_LANGUAGE);

    /* 重置接收缓冲 */
    s_http_len = 0;
    s_http_overflow = false;
    if (s_http_buf) s_http_buf[0] = '\0';  /* 不需要清零整个 512KB */

    /* 构建请求体 JSON（使用 base64 编码的 PCM）
     * 火山引擎 ASR 支持直接传 audio/raw (PCM) 字节流，也支持JSON包含base64
     * 这里用简化 JSON 方式（需base64）
     * 注：实际部署时建议使用 multipart/form-data 直传 PCM 效率更高
     */

    /* 简化实现：构建 JSON body，audio 字段用 base64 编码 */
    /* base64 编码后体积约为原来的4/3 */
    size_t b64_len = ((upload_len + 2) / 3) * 4 + 1;
    char *b64_buf = malloc(b64_len);
    if (!b64_buf) {
        ESP_LOGE(TAG, "STT: malloc failed for base64 buffer");
        if (wav_buf) free(wav_buf);
        return false;
    }

    /* 内联 base64 编码 */
    static const char b64_table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    uint32_t bi = 0;
    for (uint32_t i = 0; i < upload_len; i += 3) {
        uint32_t b = ((uint32_t)upload_data[i] << 16)
                   | ((i + 1 < upload_len) ? ((uint32_t)upload_data[i+1] << 8) : 0)
                   | ((i + 2 < upload_len) ? ((uint32_t)upload_data[i+2]) : 0);
        b64_buf[bi++] = b64_table[(b >> 18) & 0x3F];
        b64_buf[bi++] = b64_table[(b >> 12) & 0x3F];
        b64_buf[bi++] = (i + 1 < upload_len) ? b64_table[(b >> 6) & 0x3F] : '=';
        b64_buf[bi++] = (i + 2 < upload_len) ? b64_table[b & 0x3F] : '=';
    }
    b64_buf[bi] = '\0';

    /* 构建 JSON body */
    /* 注意：JSON头部（appid/token/workflow等）本身约400字节，需留足空间 */
    size_t json_len = b64_len + 1536;
    char *json_body = malloc(json_len);
    if (!json_body) {
        free(b64_buf);
        if (wav_buf) free(wav_buf);
        return false;
    }
    char reqid[40];
    snprintf(reqid, sizeof(reqid), "esp32p4_%08lx", (unsigned long)esp_random());

    int written = snprintf(json_body, json_len,
        "{"
        "\"app\":{\"appid\":\"%s\",\"token\":\"%s\",\"cluster\":\"%s\"},"
        "\"user\":{\"uid\":\"esp32p4\"},"
        "\"request\":{\"reqid\":\"%s\",\"nbest\":1,\"show_utterances\":false,"
                     "\"workflow\":\"%s\",\"sequence\":-1,"
                     "\"vad_signal\":true,"
                     "\"start_silence_time\":\"%s\","
                     "\"vad_silence_time\":\"%s\"},"
        "\"audio\":{\"format\":\"%s\",\"rate\":%d,\"bits\":16,\"channel\":1,"
                  "\"language\":\"%s\",\"data\":\"%s\"}"
        "}",
        ASR_APP_ID, ASR_ACCESS_TOKEN, ASR_CLUSTER, reqid,
        ASR_WORKFLOW, ASR_START_SILENCE_MS, ASR_VAD_SILENCE_MS,
        audio_format, RECORD_SAMPLE_RATE, ASR_LANGUAGE, b64_buf);
    free(b64_buf);
    if (written < 0 || (size_t)written >= json_len) {
        ESP_LOGE(TAG, "STT: JSON body truncated! written=%d json_len=%zu", written, json_len);
        free(json_body);
        if (wav_buf) free(wav_buf);
        return false;
    }
    ESP_LOGI(TAG, "STT: JSON body size=%d bytes", written);

    /* HTTP 请求 */
    esp_http_client_config_t cfg = {
        .url            = ASR_URL,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .event_handler  = http_event_collect,
        .buffer_size    = 4096,
        .timeout_ms     = 15000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    /* 火山引擎 ASR 鉴权格式：Bearer; {token} */
    char asr_auth[160];
    snprintf(asr_auth, sizeof(asr_auth), "Bearer; %s", ASR_ACCESS_TOKEN);
    esp_http_client_set_header(client, "Authorization", asr_auth);
    esp_http_client_set_post_field(client, json_body, strlen(json_body));

    esp_err_t err = esp_http_client_perform(client);
    int http_status = esp_http_client_get_status_code(client);
    free(json_body);
    esp_http_client_cleanup(client);
    if (wav_buf) free(wav_buf);

    ESP_LOGI(TAG, "STT HTTP status: %d", http_status);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "STT HTTP error: %s", esp_err_to_name(err));
        return false;
    }

    if (s_http_len == 0) {
        ESP_LOGE(TAG, "STT: empty response");
        return false;
    }

    /* 解析响应 JSON，提取识别文本 */
    s_http_buf[s_http_len] = '\0';
    if (s_http_overflow) {
        ESP_LOGW(TAG, "STT response truncated at %lu bytes", (unsigned long)s_http_len);
    }
    ESP_LOGI(TAG, "STT response: %.512s%s", (char*)s_http_buf,
             s_http_len > 512 ? "...(truncated)" : "");

    cJSON *root = cJSON_Parse((char*)s_http_buf);
    if (!root) {
        ESP_LOGE(TAG, "STT: JSON parse failed");
        return false;
    }

    bool success = false;
    /* 火山引擎ASR响应格式：{"result":[{"text":"..."}],...} */
    cJSON *result_obj = cJSON_GetObjectItem(root, "result");
    if (result_obj && cJSON_IsObject(result_obj)) {
        cJSON *text_obj = cJSON_GetObjectItem(result_obj, "text");
        if (text_obj && cJSON_IsString(text_obj) && text_obj->valuestring[0]) {
            strncpy(out_text, text_obj->valuestring, out_size - 1);
            out_text[out_size - 1] = '\0';
            ESP_LOGI(TAG, "STT result: %s", out_text);
            success = true;
        }
    }
    if (!success) {
        int response_code = json_extract_response_code(root);
        const char *response_message = json_extract_response_message(root);
        if (response_code != 0 && response_code != 1000) {
            ESP_LOGW(TAG, "STT server rejected audio: code=%d, message=%s",
                     response_code, response_message ? response_message : "unknown");
        }
        cJSON *result_arr = cJSON_GetObjectItem(root, "result");
        if (result_arr && cJSON_IsArray(result_arr)) {
            cJSON *first = cJSON_GetArrayItem(result_arr, 0);
            if (first) {
                cJSON *text_obj = cJSON_GetObjectItem(first, "text");
                if (text_obj && cJSON_IsString(text_obj) && text_obj->valuestring[0]) {
                    strncpy(out_text, text_obj->valuestring, out_size - 1);
                    out_text[out_size - 1] = '\0';
                    ESP_LOGI(TAG, "STT result: %s", out_text);
                    success = true;
                }
            }
        }
    }

    cJSON_Delete(root);

    if (!success) {
        ESP_LOGE(TAG, "STT: no text in response");
    }
    return success;
}

/* ==================== LLM 模块 ==================== */

/**
 * @brief 调用豆包大模型，返回回复文本
 * @param user_text      用户输入文本（STT识别结果）
 * @param memory_context 家庭记忆上下文（RAG检索结果，可为NULL表示无记忆）
 * @param out_reply      输出回复缓冲
 * @param out_size       输出缓冲大小
 * @return true=成功
 */
static bool llm_chat(const char *user_text, const char *memory_context,
                     char *out_reply, size_t out_size)
{
    if (!user_text || user_text[0] == '\0') return false;

    ESP_LOGI(TAG, "LLM: user said: %s", user_text);

    s_http_len = 0;
    s_http_overflow = false;
    if (s_http_buf) s_http_buf[0] = '\0';  /* 不需要清零整个 512KB */

    /*
     * system prompt 构成：
     *   固定角色描述
     *   + 家庭记忆片段（如果 RAG 有返回）
     *
     * 有记忆时：豆包能回答"我什么时候吃药""女儿叫什么名字"等具体问题
     * 无记忆时：豆包作为通用陪伴助手回答
     */
    const char *base_prompt =
        "你是陪伴老人的温暖智能助手小柚子。"
        "请用简洁、亲切、口语化的语言回答，不超过60字，不使用Markdown格式。"
        "如果问到家人或生活的具体情况，请根据提供的家庭记忆来回答。";

    /* 动态拼接 system prompt */
    char *system_prompt = malloc(2048);
    if (!system_prompt) return false;

    if (memory_context && memory_context[0] != '\0') {
        /* 有记忆：把记忆片段截断到 RAG_CONTEXT_MAX 字符，拼入 prompt */
        char trimmed_ctx[RAG_CONTEXT_MAX + 1];
        strncpy(trimmed_ctx, memory_context, RAG_CONTEXT_MAX);
        trimmed_ctx[RAG_CONTEXT_MAX] = '\0';
        snprintf(system_prompt, 2048, "%s\n\n%s", base_prompt, trimmed_ctx);
    } else {
        /* 无记忆：使用基础 prompt */
        strncpy(system_prompt, base_prompt, 2047);
        system_prompt[2047] = '\0';
    }

    /* 对 system_prompt 和 user_text 中的特殊字符转义，确保 JSON 合法 */
    /* 简单处理：将双引号替换为中文引号，避免 JSON 破坏 */
    for (char *p = system_prompt; *p; p++) {
        if (*p == '"') *p = '\x27';  /* 替换为单引号 */
    }

    char *post_data = malloc(4096);
    if (!post_data) {
        free(system_prompt);
        return false;
    }

    /* user_text 的双引号处理 */
    char safe_user[300];
    size_t ui = 0, si = 0;
    while (user_text[ui] && si < sizeof(safe_user) - 2) {
        if (user_text[ui] == '"' || user_text[ui] == '\\') safe_user[si++] = '\\';
        safe_user[si++] = user_text[ui++];
    }
    safe_user[si] = '\0';

    snprintf(post_data, 4096,
        "{\"model\":\"%s\","
        "\"messages\":["
        "{\"role\":\"system\",\"content\":\"%s\"},"
        "{\"role\":\"user\",\"content\":\"%s\"}"
        "]}",
        DOUBAO_ENDPOINT, system_prompt, safe_user);

    free(system_prompt);

    esp_http_client_config_t cfg = {
        .url            = "https://ark.cn-beijing.volces.com/api/v3/chat/completions",
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .event_handler  = http_event_collect,
        .buffer_size    = 4096,
        .timeout_ms     = 20000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");

    char auth_hdr[128];
    snprintf(auth_hdr, sizeof(auth_hdr), "Bearer %s", VOLCENGINE_API_KEY);
    esp_http_client_set_header(client, "Authorization", auth_hdr);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);
    free(post_data);
    esp_http_client_cleanup(client);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LLM HTTP error: %s", esp_err_to_name(err));
        return false;
    }

    if (s_http_len == 0) return false;
    s_http_buf[s_http_len] = '\0';
    if (s_http_overflow) {
        ESP_LOGW(TAG, "LLM response truncated at %lu bytes", (unsigned long)s_http_len);
    }

    /* 解析豆包响应：{"choices":[{"message":{"content":"..."}}]} */
    cJSON *root = cJSON_Parse((char*)s_http_buf);
    if (!root) {
        ESP_LOGE(TAG, "LLM: JSON parse failed");
        return false;
    }

    bool success = false;
    cJSON *choices = cJSON_GetObjectItem(root, "choices");
    if (choices && cJSON_IsArray(choices)) {
        cJSON *choice0 = cJSON_GetArrayItem(choices, 0);
        if (choice0) {
            cJSON *msg = cJSON_GetObjectItem(choice0, "message");
            if (msg) {
                cJSON *content = cJSON_GetObjectItem(msg, "content");
                if (content && cJSON_IsString(content) && content->valuestring[0]) {
                    strncpy(out_reply, content->valuestring, out_size - 1);
                    out_reply[out_size - 1] = '\0';
                    ESP_LOGI(TAG, "LLM reply: %s", out_reply);
                    success = true;
                }
            }
        }
    }

    cJSON_Delete(root);
    return success;
}

/* ==================== TTS 模块 ==================== */

/**
 * @brief 调用火山引擎 TTS，获取 PCM 音频并通过 ES8311 播放
 * @param text  要合成的文本
 * @return true=成功
 */
static bool tts_speak(const char *text)
{
    if (!text || text[0] == '\0') return false;

    const char *tts_voice_type = "BV001_streaming";
    const char *tts_language = "cn";

    ESP_LOGI(TAG, "TTS: synthesizing: %s", text);

    s_http_len = 0;
    s_http_overflow = false;
    if (s_http_buf) s_http_buf[0] = '\0';  /* 不需要清零整个 512KB */

    /* 构建请求体 */
    size_t text_len = strlen(text);
    char *safe_text = malloc(text_len * 2 + 1);
    if (!safe_text) return false;

    size_t si = 0;
    for (size_t i = 0; i < text_len && si < text_len * 2; i++) {
        char ch = text[i];
        if (ch == '"' || ch == '\\') {
            safe_text[si++] = '\\';
            safe_text[si++] = ch;
        } else if (ch == '\n') {
            safe_text[si++] = '\\';
            safe_text[si++] = 'n';
        } else if (ch == '\r') {
            safe_text[si++] = '\\';
            safe_text[si++] = 'r';
        } else {
            safe_text[si++] = ch;
        }
    }
    safe_text[si] = '\0';

    char reqid[40];
    snprintf(reqid, sizeof(reqid), "tts_%08lx", (unsigned long)esp_random());

    size_t body_cap = strlen(safe_text) + 1024;
    char *json_body = malloc(body_cap);
    if (!json_body) {
        free(safe_text);
        return false;
    }

    snprintf(json_body, body_cap,
        "{"
        "\"app\":{\"appid\":\"%s\",\"token\":\"%s\",\"cluster\":\"volcano_tts\"},"
        "\"user\":{\"uid\":\"esp32p4\"},"
        "\"audio\":{\"voice_type\":\"%s\",\"encoding\":\"pcm\",\"rate\":%d,"
                  "\"speed_ratio\":1.0,\"volume_ratio\":1.0,\"pitch_ratio\":1.0,"
                  "\"language\":\"%s\"},"
        "\"request\":{\"reqid\":\"%s\",\"text\":\"%s\",\"text_type\":\"plain\","
                   "\"operation\":\"query\",\"silence_duration\":\"125\"}"
        "}",
        TTS_APP_ID, TTS_ACCESS_TOKEN,
        tts_voice_type, RECORD_SAMPLE_RATE, tts_language, reqid, safe_text);
    free(safe_text);

    esp_http_client_config_t cfg = {
        .url            = TTS_URL,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .event_handler  = http_event_collect,
        .buffer_size    = 4096,
        .timeout_ms     = 15000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    /* 火山引擎 TTS 鉴权格式：Bearer; {token} */
    char tts_auth[160];
    snprintf(tts_auth, sizeof(tts_auth), "Bearer; %s", TTS_ACCESS_TOKEN);
    esp_http_client_set_header(client, "Authorization", tts_auth);
    esp_http_client_set_post_field(client, json_body, strlen(json_body));

    esp_err_t err = esp_http_client_perform(client);
    int http_status = esp_http_client_get_status_code(client);
    free(json_body);
    esp_http_client_cleanup(client);

    ESP_LOGI(TAG, "TTS HTTP status: %d", http_status);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "TTS HTTP error: %s", esp_err_to_name(err));
        return false;
    }

    if (s_http_len == 0) {
        ESP_LOGE(TAG, "TTS: empty response");
        return false;
    }
    s_http_buf[s_http_len] = '\0';
    if (s_http_overflow) {
        ESP_LOGW(TAG, "TTS response truncated at %lu bytes", (unsigned long)s_http_len);
    }
    /* 不打印完整 TTS 响应（base64 音频可能几十~几百KB，会爆栈） */
    ESP_LOGI(TAG, "TTS response received: %lu bytes", (unsigned long)s_http_len);

    /* TTS 响应 JSON：{"data":"<base64_pcm>", ...} */
    cJSON *root = cJSON_Parse((char*)s_http_buf);
    if (!root) {
        ESP_LOGE(TAG, "TTS: JSON parse failed");
        return false;
    }

    int response_code = json_extract_response_code(root);
    const char *response_message = json_extract_response_message(root);
    if (response_code != 0 && response_code != 3000) {
        ESP_LOGW(TAG, "TTS server rejected request: code=%d, message=%s",
                 response_code, response_message ? response_message : "unknown");
    }

    cJSON *data_obj = cJSON_GetObjectItem(root, "data");
    if (!data_obj || !cJSON_IsString(data_obj) || !data_obj->valuestring[0]) {
        ESP_LOGE(TAG, "TTS: no audio data in response");
        cJSON_Delete(root);
        return false;
    }

    /* Base64 解码 */
    const char *b64 = data_obj->valuestring;
    size_t b64_len = strlen(b64);
    size_t pcm_len = (b64_len / 4) * 3;
    uint8_t *pcm_buf = heap_caps_malloc(pcm_len + 4, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!pcm_buf) {
        cJSON_Delete(root);
        return false;
    }

    /* 内联 base64 解码 */
    static const uint8_t b64_dec[256] = {
        ['A']=0, ['B']=1, ['C']=2, ['D']=3, ['E']=4, ['F']=5, ['G']=6, ['H']=7,
        ['I']=8, ['J']=9, ['K']=10,['L']=11,['M']=12,['N']=13,['O']=14,['P']=15,
        ['Q']=16,['R']=17,['S']=18,['T']=19,['U']=20,['V']=21,['W']=22,['X']=23,
        ['Y']=24,['Z']=25,['a']=26,['b']=27,['c']=28,['d']=29,['e']=30,['f']=31,
        ['g']=32,['h']=33,['i']=34,['j']=35,['k']=36,['l']=37,['m']=38,['n']=39,
        ['o']=40,['p']=41,['q']=42,['r']=43,['s']=44,['t']=45,['u']=46,['v']=47,
        ['w']=48,['x']=49,['y']=50,['z']=51,['0']=52,['1']=53,['2']=54,['3']=55,
        ['4']=56,['5']=57,['6']=58,['7']=59,['8']=60,['9']=61,['+']=62,['/']=63
    };

    size_t out_len = 0;
    for (size_t i = 0; i < b64_len; i += 4) {
        uint32_t v = ((uint32_t)b64_dec[(uint8_t)b64[i]]   << 18)
                   | ((uint32_t)b64_dec[(uint8_t)b64[i+1]] << 12)
                   | ((b64[i+2] != '=') ? ((uint32_t)b64_dec[(uint8_t)b64[i+2]] << 6) : 0)
                   | ((b64[i+3] != '=') ? ((uint32_t)b64_dec[(uint8_t)b64[i+3]]) : 0);
        pcm_buf[out_len++] = (v >> 16) & 0xFF;
        if (b64[i+2] != '=') pcm_buf[out_len++] = (v >> 8) & 0xFF;
        if (b64[i+3] != '=') pcm_buf[out_len++] = v & 0xFF;
    }
    cJSON_Delete(root);

    ESP_LOGI(TAG, "TTS: playing %zu bytes of PCM audio", out_len);

    /* 调整 I2S 采样率以匹配 TTS 输出 */
    i2s_set_samplerate_bits_sample(RECORD_SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT);

    /* 播放 PCM 数据：TTS 返回单声道，I2S 输出为立体声，需要 mono→stereo 转换 */
    int16_t *mono = (int16_t *)pcm_buf;
    uint32_t mono_samples = out_len / 2;
    const int BLOCK = 512;
    int16_t stereo_block[BLOCK * 2];
    for (uint32_t i = 0; i < mono_samples; i += BLOCK) {
        int count = ((mono_samples - i) < BLOCK) ? (mono_samples - i) : BLOCK;
        for (int j = 0; j < count; j++) {
            stereo_block[j * 2]     = mono[i + j];  /* L */
            stereo_block[j * 2 + 1] = mono[i + j];  /* R */
        }
        i2s_tx_write((uint8_t *)stereo_block, count * 4);
    }

    free(pcm_buf);
    ESP_LOGI(TAG, "TTS: playback complete");
    return true;
}

/* ==================== 录音模块 ==================== */

/**
 * @brief 录音（使用 INMP441 数字麦克风，静音自动停止）
 * @return 实际录音字节数（单声道 16bit PCM）
 */
static uint32_t record_audio(void)
{
    ESP_LOGI(TAG, "Recording... (max %d seconds)", RECORD_MAX_SECONDS);

    if (!s_record_buf) return 0;

    /* INMP441 直接输出单声道 16bit 数据，不需要立体声转换 */
    uint8_t *chunk_buf = malloc(CHUNK_BYTES);
    if (!chunk_buf) return 0;

    uint32_t total_bytes = 0;
    uint32_t max_bytes = RECORD_SAMPLE_RATE * 2 * RECORD_MAX_SECONDS;  /* 16bit mono */
    uint32_t silence_ms = 0;
    uint32_t speech_ms = 0;
    bool got_speech = false;
    uint32_t chunk_ms = (CHUNK_SAMPLES * 1000) / RECORD_SAMPLE_RATE;

    while (total_bytes < max_bytes) {
        size_t read_bytes = inmp441_read(chunk_buf, CHUNK_BYTES);
        if (read_bytes == 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        /* 直接复制到录音缓冲（已经是单声道 16bit）*/
        uint32_t to_copy = read_bytes;
        if (total_bytes + to_copy > max_bytes) {
            to_copy = max_bytes - total_bytes;
        }
        memcpy(s_record_buf + total_bytes, chunk_buf, to_copy);
        total_bytes += to_copy;

        /* 静音检测 */
        uint32_t rms = calc_rms((int16_t*)chunk_buf, read_bytes / 2);
        if (rms >= SPEECH_THRESHOLD) {
            speech_ms += chunk_ms;
            silence_ms = 0;
            if (!got_speech) {
                got_speech = true;
                ESP_LOGI(TAG, "Speech detected (RMS=%lu)", (unsigned long)rms);
            }
        } else if (got_speech) {
            if (rms < END_SILENCE_THRESHOLD) {
                silence_ms += chunk_ms;
            } else {
                silence_ms += chunk_ms / 2;
            }

            if (speech_ms >= MIN_SPEECH_MS &&
                silence_ms >= END_SILENCE_DURATION_MS &&
                total_bytes > RECORD_SAMPLE_RATE) {
                ESP_LOGI(TAG,
                         "Speech finished, stopping recording (speech_ms=%lu, tail_silence_ms=%lu, rms=%lu)",
                         (unsigned long)speech_ms,
                         (unsigned long)silence_ms,
                         (unsigned long)rms);
                break;
            }
        }
    }

    free(chunk_buf);

    if (!got_speech && total_bytes > 0) {
        ESP_LOGW(TAG, "No speech detected before recording timeout");
    }

    if (0 && total_bytes > 0) {
        int16_t *samples_ptr = (int16_t*)s_record_buf;
        uint32_t num_samples = total_bytes / 2;

        /* DC 偏移去除 */
        int64_t sum = 0;
        for (uint32_t i = 0; i < num_samples; i++) {
            sum += samples_ptr[i];
        }
        int16_t dc_offset = (int16_t)(sum / (int64_t)num_samples);
        for (uint32_t i = 0; i < num_samples; i++) {
            samples_ptr[i] -= dc_offset;
        }
        ESP_LOGI(TAG, "DC offset removed: %d", dc_offset);

        /* 归一化：将峰值放大到接近满幅，让 ASR 更容易识别 */
        int16_t peak = 0;
        for (uint32_t i = 0; i < num_samples; i++) {
            int16_t abs_val = (samples_ptr[i] < 0) ? -samples_ptr[i] : samples_ptr[i];
            if (abs_val > peak) peak = abs_val;
        }
        if (peak > 200 && peak < 23000) {
            float gain = 23000.0f / (float)peak;
            for (uint32_t i = 0; i < num_samples; i++) {
                int32_t v = (int32_t)(samples_ptr[i] * gain);
                if (v > 32767) v = 32767;
                if (v < -32768) v = -32768;
                samples_ptr[i] = (int16_t)v;
            }
            ESP_LOGI(TAG, "Audio normalized: peak=%d, gain=%.1fx", peak, gain);
        }

        uint32_t mono_rms = calc_rms(samples_ptr, num_samples);
        ESP_LOGI(TAG, "Recorded %lu bytes (%.1f seconds), final RMS=%lu",
                 total_bytes,
                 (float)total_bytes / (RECORD_SAMPLE_RATE * 2),
                 mono_rms);
    }
    if (total_bytes > 0) {
        total_bytes = preprocess_recorded_audio(s_record_buf, total_bytes);
    }
    return total_bytes;
}

/* ==================== 主流水线任务 ==================== */

static void voice_pipeline_task(void *arg)
{
    /* 分配 HTTP 接收缓冲（放到 PSRAM）*/
    s_http_buf = heap_caps_malloc(HTTP_RECV_BUF_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_http_buf) {
        ESP_LOGE(TAG, "Failed to allocate HTTP recv buffer");
        vTaskDelete(NULL);
        return;
    }

    /* 分配 RAG 接收缓冲（独立，放到 PSRAM）*/
    s_rag_buf = heap_caps_malloc(RAG_RECV_BUF_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_rag_buf) {
        ESP_LOGW(TAG, "Failed to allocate RAG buffer, RAG disabled");
        /* 不致命，继续运行，只是没有记忆功能 */
    }

    /* 分配录音缓冲（放到 PSRAM）*/
    s_record_buf = heap_caps_malloc(RECORD_BUF_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_record_buf) {
        ESP_LOGE(TAG, "Failed to allocate record buffer");
        free(s_http_buf);
        vTaskDelete(NULL);
        return;
    }

    /* 探测香橙派 RAG 服务 */
    if (s_rag_buf) {
        ESP_LOGI(TAG, "Probing RAG service at %s:%d ...", RAG_SERVICE_IP, RAG_SERVICE_PORT);
        s_rag_available = rag_health_check();
        if (s_rag_available) {
            ESP_LOGI(TAG, "RAG service available — 家庭记忆功能已启用");
        } else {
            ESP_LOGW(TAG, "RAG service unavailable — 将以无记忆模式运行（香橙派未启动？）");
        }
    }

    char stt_text[256]              = {0};
    char llm_reply[512]             = {0};
    char memory_context[RAG_CONTEXT_MAX + 1] = {0};

    while (1) {
        /* 等待唤醒信号 */
        xSemaphoreTake(s_trigger_sem, portMAX_DELAY);
        runtime_lock(portMAX_DELAY);
        ESP_LOGI(TAG, "=== Voice pipeline triggered ===");
        voice_camera_pause(true);

        /* 通知 avatar 界面：语音开始，阻止自动跳转 */
        avatar_set_voice_active(true);

        /* Step 1: 录音（通过 INMP441 数字麦克风）*/
        set_state(VOICE_STATE_LISTENING);
        memset(s_record_buf, 0, RECORD_BUF_SIZE);
        s_record_len = record_audio();

        if (s_record_len < RECORD_SAMPLE_RATE * 2 / 2) {
            ESP_LOGW(TAG, "Recording too short, ignoring");
            avatar_set_voice_active(false);
            voice_camera_pause(false);
            set_state(VOICE_STATE_IDLE);
            runtime_unlock();
            continue;
        }

        /* Step 1.5: 回放录音（确认麦克风采集正常）*/
        if (0) {
            ESP_LOGI(TAG, "Playing back recording (%lu bytes)...", s_record_len);
            i2s_set_samplerate_bits_sample(RECORD_SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT);
            int16_t *mono = (int16_t*)s_record_buf;
            uint32_t mono_samples = s_record_len / 2;
            /* 单声道转立体声，分块写入 */
            const int BLOCK = 512;
            int16_t stereo_block[BLOCK * 2];
            for (uint32_t i = 0; i < mono_samples; i += BLOCK) {
                int count = (mono_samples - i) < BLOCK ? (mono_samples - i) : BLOCK;
                for (int j = 0; j < count; j++) {
                    stereo_block[j * 2]     = mono[i + j];
                    stereo_block[j * 2 + 1] = mono[i + j];
                }
                i2s_tx_write((uint8_t*)stereo_block, count * 4);
            }
            ESP_LOGI(TAG, "Playback done");
        }

        /* Step 2: STT */
        set_state(VOICE_STATE_PROCESSING);
        memset(stt_text, 0, sizeof(stt_text));

        bool stt_ok = stt_recognize(s_record_buf, s_record_len, stt_text, sizeof(stt_text));
        if (!stt_ok || stt_text[0] == '\0') {
            ESP_LOGW(TAG, "STT failed or no speech detected");
            avatar_set_voice_active(false);
            voice_camera_pause(false);
            set_state(VOICE_STATE_IDLE);
            runtime_unlock();
            continue;
        }

        /* Step 3: RAG — 查询家庭记忆（可选，失败不阻断流程）
         *
         * 如果 RAG 之前标记不可用，每隔 5 次触发重试一次 health check，
         * 这样香橙派重启后无需重启设备就能自动恢复。
         */
        memset(memory_context, 0, sizeof(memory_context));
        if (!s_rag_available && s_rag_buf) {
            static uint8_t rag_retry_count = 0;
            if (++rag_retry_count >= 5) {
                rag_retry_count = 0;
                ESP_LOGI(TAG, "Retrying RAG health check...");
                s_rag_available = rag_health_check();
            }
        }
        bool has_memory = rag_query(stt_text, memory_context, sizeof(memory_context));
        if (has_memory) {
            ESP_LOGI(TAG, "RAG: injecting %zu chars of family memory into LLM prompt",
                     strlen(memory_context));
        }

        /* Step 4: LLM（带记忆上下文）*/
        memset(llm_reply, 0, sizeof(llm_reply));
        bool llm_ok = llm_chat(stt_text,
                               has_memory ? memory_context : NULL,
                               llm_reply, sizeof(llm_reply));
        if (!llm_ok || llm_reply[0] == '\0') {
            ESP_LOGW(TAG, "LLM failed");
            avatar_set_voice_active(false);
            voice_camera_pause(false);
            set_state(VOICE_STATE_IDLE);
            runtime_unlock();
            continue;
        }

        /* Step 5: TTS 播放 */
        set_state(VOICE_STATE_SPEAKING);
        tts_speak(llm_reply);

        avatar_set_voice_active(false);
        voice_camera_pause(false);
        set_state(VOICE_STATE_IDLE);
        runtime_unlock();
        ESP_LOGI(TAG, "=== Voice pipeline complete ===");
    }
}

/* ==================== 对外接口 ==================== */

esp_err_t voice_pipeline_init(void)
{
    ESP_LOGI(TAG, "Initializing voice pipeline...");
    ESP_LOGI(TAG, "ASR config: url=%s, cluster=%s, workflow=%s",
             ASR_URL, ASR_CLUSTER, ASR_WORKFLOW);

    /* 初始化 I2S */
    esp_err_t ret = myi2s_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2S init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* 初始化 ES8311（共享 I2C_NUM_0, GPIO28/29，仅用于播放）*/
    ret = myes8311_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ES8311 init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* 初始化 INMP441 数字麦克风（I2S_NUM_1，专门用于录音）*/
    ret = inmp441_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "INMP441 init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* 喇叭测试：播放 440Hz 正弦波 0.5 秒，听到"嘟"声说明喇叭正常 */
    {
        vTaskDelay(pdMS_TO_TICKS(200));   /* 等待 I2S/ES8311 时钟稳定 */
        const int sample_rate = 16000;
        const int duration_ms = 500;
        const int total_samples = sample_rate * duration_ms / 1000;
        const float freq = 440.0f;
        int16_t *test_buf = heap_caps_malloc(total_samples * 2 * sizeof(int16_t), MALLOC_CAP_SPIRAM);
        if (test_buf) {
            for (int i = 0; i < total_samples; i++) {
                int16_t v = (int16_t)(32767 * 0.5f * sinf(2.0f * M_PI * freq * i / sample_rate));
                test_buf[i * 2]     = v;  /* L */
                test_buf[i * 2 + 1] = v;  /* R */
            }
            i2s_set_samplerate_bits_sample(sample_rate, I2S_DATA_BIT_WIDTH_16BIT);
            size_t w = i2s_tx_write((uint8_t *)test_buf, total_samples * 2 * sizeof(int16_t));
            heap_caps_free(test_buf);
            ESP_LOGI(TAG, "Speaker test done (written=%d bytes). Heard a beep? -> speaker OK.", (int)w);
        } else {
            ESP_LOGW(TAG, "Speaker test: malloc failed");
        }
    }

    /* INMP441 麦克风测试：采集 0.5 秒，检查 RMS 能量 */
    {
        vTaskDelay(pdMS_TO_TICKS(200));
        const int test_ms = 500;
        const int test_bytes = RECORD_SAMPLE_RATE * 2 * test_ms / 1000;
        uint8_t *mic_buf = heap_caps_malloc(test_bytes, MALLOC_CAP_SPIRAM);
        if (mic_buf) {
            size_t total = 0;
            while (total < test_bytes) {
                size_t rd = inmp441_read(mic_buf + total, test_bytes - total);
                if (rd == 0) { vTaskDelay(pdMS_TO_TICKS(10)); continue; }
                total += rd;
            }
            uint32_t rms = calc_rms((int16_t*)mic_buf, total / 2);
            int16_t *d = (int16_t*)mic_buf;
            ESP_LOGI(TAG, "Mic test: RMS=%lu, PCM[0..7]=%d %d %d %d %d %d %d %d",
                     rms, d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7]);
            if (rms > 50) {
                ESP_LOGI(TAG, "INMP441 mic OK! (RMS=%lu)", rms);
            } else {
                ESP_LOGW(TAG, "INMP441 mic may not be connected (RMS=%lu)", rms);
            }
            heap_caps_free(mic_buf);
        }
    }

    /* 创建唤醒信号量 */
    s_runtime_mutex = xSemaphoreCreateMutex();
    if (!s_runtime_mutex) {
        return ESP_ERR_NO_MEM;
    }

    s_trigger_sem = xSemaphoreCreateBinary();
    if (!s_trigger_sem) {
        return ESP_ERR_NO_MEM;
    }

    /* 启动流水线任务（高优先级，分配到较大栈）*/
    BaseType_t res = xTaskCreatePinnedToCore(
        voice_pipeline_task,
        "voice_pipeline",
        1024 * 48,       /* 48KB 栈，base64+cJSON+HTTP需要较大栈空间 */
        NULL,
        6,               /* 优先级高于UI（5），低于系统任务 */
        &s_pipeline_task,
        1                /* 核心1，不与摄像头/UI抢占核心0 */
    );

    if (res != pdPASS) {
        ESP_LOGE(TAG, "Failed to create pipeline task");
        return ESP_ERR_NO_MEM;
    }

    s_voice_ready = true;
    ESP_LOGI(TAG, "Voice pipeline initialized. Call voice_pipeline_trigger() to wake.");
    return ESP_OK;
}

void voice_pipeline_set_state_cb(voice_state_cb_t cb)
{
    s_state_cb = cb;
}

void voice_pipeline_trigger(void)
{
    if (!s_voice_ready) {
        ESP_LOGW(TAG, "Pipeline not ready yet");
        return;
    }
    if (s_state != VOICE_STATE_IDLE) {
        BaseType_t queued = xSemaphoreGive(s_trigger_sem);
        ESP_LOGW(TAG, "Pipeline busy, queued trigger=%s (state=%d)",
                 queued == pdTRUE ? "true" : "false", s_state);
        return;
    }
    ESP_LOGI(TAG, "Trigger received");
    xSemaphoreGive(s_trigger_sem);
}

voice_state_t voice_pipeline_get_state(void)
{
    return s_state;
}

esp_err_t voice_pipeline_play_notification_chime(void)
{
    if (!s_voice_ready) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!runtime_lock(pdMS_TO_TICKS(2000))) {
        return ESP_ERR_TIMEOUT;
    }

    set_state(VOICE_STATE_SPEAKING);
    voice_camera_pause(true);
    esp_err_t ret = play_notification_chime_locked();
    voice_camera_pause(false);
    set_state(VOICE_STATE_IDLE);
    runtime_unlock();
    return ret;
}

esp_err_t voice_pipeline_speak_text(const char *text)
{
    if (!s_voice_ready) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!text || text[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }
    if (!runtime_lock(pdMS_TO_TICKS(10000))) {
        return ESP_ERR_TIMEOUT;
    }

    set_state(VOICE_STATE_SPEAKING);
    voice_camera_pause(true);
    bool ok = tts_speak(text);
    voice_camera_pause(false);
    set_state(VOICE_STATE_IDLE);
    runtime_unlock();
    return ok ? ESP_OK : ESP_FAIL;
}
