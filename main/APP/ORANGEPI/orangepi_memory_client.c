/**
 ******************************************************************************
 * @file        orangepi_memory_client.c
 * @brief       香橙派家庭记忆中枢客户端实现
 ******************************************************************************
 */

#include "orangepi_memory_client.h"
#include "cJSON.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_random.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static const char *TAG = "ORANGEPI_MEM";

#define ORANGEPI_SERVICE_IP         "192.168.43.26"
#define ORANGEPI_SERVICE_PORT       8765
#define ORANGEPI_TIMEOUT_MS         3500
#define ORANGEPI_HTTP_BUF_SIZE      4096

#define ORANGEPI_HEALTH_URL         "http://" ORANGEPI_SERVICE_IP ":" "8765" "/api/v1/health"
#define ORANGEPI_COMPOSE_URL        "http://" ORANGEPI_SERVICE_IP ":" "8765" "/api/v1/rag/compose"
#define ORANGEPI_UPSERT_URL         "http://" ORANGEPI_SERVICE_IP ":" "8765" "/api/v1/memories/upsert"

typedef struct {
    char *buffer;
    size_t capacity;
    size_t length;
    bool overflow;
} http_collect_ctx_t;

static volatile bool s_service_available = false;

static esp_err_t orangepi_http_collect(esp_http_client_event_t *evt)
{
    if (evt->event_id != HTTP_EVENT_ON_DATA || !evt->user_data) {
        return ESP_OK;
    }

    http_collect_ctx_t *ctx = (http_collect_ctx_t *)evt->user_data;
    if (!ctx->buffer || ctx->capacity == 0) {
        return ESP_OK;
    }

    size_t space_left = (ctx->capacity - 1 > ctx->length) ? (ctx->capacity - 1 - ctx->length) : 0;
    size_t to_copy = (evt->data_len <= space_left) ? evt->data_len : space_left;
    if (to_copy > 0) {
        memcpy(ctx->buffer + ctx->length, evt->data, to_copy);
        ctx->length += to_copy;
        ctx->buffer[ctx->length] = '\0';
    }
    if (to_copy < (size_t)evt->data_len) {
        ctx->overflow = true;
    }
    return ESP_OK;
}

static esp_err_t orangepi_http_json_request(const char *url,
                                            esp_http_client_method_t method,
                                            const char *body,
                                            char *response_buf,
                                            size_t response_cap,
                                            int *out_status)
{
    http_collect_ctx_t ctx = {
        .buffer = response_buf,
        .capacity = response_cap,
        .length = 0,
        .overflow = false,
    };

    if (response_buf && response_cap > 0) {
        response_buf[0] = '\0';
    }

    esp_http_client_config_t cfg = {
        .url = url,
        .timeout_ms = ORANGEPI_TIMEOUT_MS,
        .buffer_size = 2048,
        .event_handler = orangepi_http_collect,
        .user_data = &ctx,
    };
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) {
        return ESP_FAIL;
    }

    esp_http_client_set_method(client, method);
    if (body) {
        esp_http_client_set_header(client, "Content-Type", "application/json");
        esp_http_client_set_post_field(client, body, strlen(body));
    }

    esp_err_t err = esp_http_client_perform(client);
    if (out_status) {
        *out_status = esp_http_client_get_status_code(client);
    }
    esp_http_client_cleanup(client);

    if (ctx.overflow) {
        ESP_LOGW(TAG, "OrangePi response truncated at %u bytes", (unsigned int)ctx.length);
    }
    return err;
}

static void orangepi_mark_service(bool available)
{
    s_service_available = available;
}

void orangepi_memory_client_init(void)
{
    s_service_available = false;
}

bool orangepi_memory_client_is_available(void)
{
    return s_service_available;
}

bool orangepi_memory_client_probe(void)
{
    char response[512];
    int status = 0;
    esp_err_t err = orangepi_http_json_request(
        ORANGEPI_HEALTH_URL,
        HTTP_METHOD_GET,
        NULL,
        response,
        sizeof(response),
        &status
    );

    bool ok = (err == ESP_OK && status == 200);
    orangepi_mark_service(ok);
    ESP_LOGI(TAG, "Health check: %s (http=%d)", ok ? "OK" : "FAIL", status);
    return ok;
}

bool orangepi_memory_client_compose_prompt(const char *query, orangepi_compose_result_t *out_result)
{
    if (!query || query[0] == '\0' || !out_result || !s_service_available) {
        return false;
    }

    memset(out_result, 0, sizeof(*out_result));

    cJSON *root = cJSON_CreateObject();
    if (!root) {
        return false;
    }
    cJSON_AddStringToObject(root, "query", query);
    cJSON_AddStringToObject(root, "session_mode", "elder_companion");
    cJSON_AddNumberToObject(root, "max_prompt_chars", ORANGEPI_SYSTEM_PROMPT_MAX);
    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!body) {
        return false;
    }

    char response[ORANGEPI_HTTP_BUF_SIZE];
    int status = 0;
    esp_err_t err = orangepi_http_json_request(
        ORANGEPI_COMPOSE_URL,
        HTTP_METHOD_POST,
        body,
        response,
        sizeof(response),
        &status
    );
    free(body);

    if (err != ESP_OK || status != 200) {
        ESP_LOGW(TAG, "Compose failed (err=%s, http=%d)", esp_err_to_name(err), status);
        orangepi_mark_service(false);
        return false;
    }

    cJSON *resp = cJSON_Parse(response);
    if (!resp) {
        ESP_LOGE(TAG, "Compose response JSON parse failed");
        orangepi_mark_service(false);
        return false;
    }

    cJSON *prompt = cJSON_GetObjectItem(resp, "system_prompt");
    cJSON *trace_id = cJSON_GetObjectItem(resp, "trace_id");
    cJSON *elapsed = cJSON_GetObjectItem(resp, "elapsed_ms");
    cJSON *degraded = cJSON_GetObjectItem(resp, "degraded");
    cJSON *has_memory = cJSON_GetObjectItem(resp, "has_memory");

    if (!cJSON_IsString(prompt) || !prompt->valuestring || prompt->valuestring[0] == '\0') {
        cJSON_Delete(resp);
        orangepi_mark_service(false);
        return false;
    }

    strncpy(out_result->system_prompt, prompt->valuestring, ORANGEPI_SYSTEM_PROMPT_MAX);
    out_result->system_prompt[ORANGEPI_SYSTEM_PROMPT_MAX] = '\0';
    if (cJSON_IsString(trace_id) && trace_id->valuestring) {
        strncpy(out_result->trace_id, trace_id->valuestring, ORANGEPI_TRACE_ID_MAX - 1);
        out_result->trace_id[ORANGEPI_TRACE_ID_MAX - 1] = '\0';
    }
    out_result->elapsed_ms = cJSON_IsNumber(elapsed) ? elapsed->valueint : -1;
    out_result->degraded = cJSON_IsBool(degraded) ? cJSON_IsTrue(degraded) : false;
    out_result->has_memory = cJSON_IsBool(has_memory) ? cJSON_IsTrue(has_memory) : false;
    out_result->success = true;

    cJSON_Delete(resp);
    orangepi_mark_service(true);
    return true;
}

esp_err_t orangepi_memory_client_upsert_note(const char *sender, const char *message)
{
    if (!sender || sender[0] == '\0' || !message || message[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON *items = cJSON_AddArrayToObject(root, "items");
    cJSON *item = cJSON_CreateObject();
    char note_id[40];
    char content[320];

    if (!root || !items || !item) {
        cJSON_Delete(root);
        return ESP_ERR_NO_MEM;
    }

    snprintf(note_id, sizeof(note_id), "note_%08lx", (unsigned long)esp_random());
    snprintf(content, sizeof(content), "发送人：%s\n内容：%s", sender, message);

    cJSON_AddStringToObject(item, "id", note_id);
    cJSON_AddStringToObject(item, "category", "note");
    cJSON_AddStringToObject(item, "title", "家属留言");
    cJSON_AddStringToObject(item, "content", content);
    cJSON_AddStringToObject(item, "source", "mqtt_app");
    cJSON_AddNumberToObject(item, "priority", 70);

    cJSON *keywords = cJSON_AddArrayToObject(item, "keywords");
    if (!keywords) {
        cJSON_Delete(root);
        return ESP_ERR_NO_MEM;
    }
    cJSON_AddItemToArray(keywords, cJSON_CreateString(sender));
    cJSON_AddItemToArray(keywords, cJSON_CreateString("家属留言"));
    cJSON_AddItemToArray(keywords, cJSON_CreateString("来看你"));
    cJSON_AddItemToArray(items, item);

    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!body) {
        return ESP_ERR_NO_MEM;
    }

    char response[768];
    int status = 0;
    esp_err_t err = orangepi_http_json_request(
        ORANGEPI_UPSERT_URL,
        HTTP_METHOD_POST,
        body,
        response,
        sizeof(response),
        &status
    );
    free(body);

    if (err != ESP_OK || status != 200) {
        ESP_LOGW(TAG, "Upsert note failed (err=%s, http=%d)", esp_err_to_name(err), status);
        if (err != ESP_OK) {
            orangepi_mark_service(false);
        }
        return err == ESP_OK ? ESP_FAIL : err;
    }

    orangepi_mark_service(true);
    ESP_LOGI(TAG, "Note synced to OrangePi memory service");
    return ESP_OK;
}
