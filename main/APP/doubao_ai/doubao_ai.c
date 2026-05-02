#include "doubao_ai.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VOLCENGINE_API_KEY  "89c5ffe9-8cba-466a-9007-f9ebc4817b7f"
#define VOLCENGINE_ENDPOINT "ep-20260310180907-frhfb"
static const char *TAG = "DOUBAO_AI";

// HTTP 接收回调函数：打印豆包回复
static esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        printf("%.*s", evt->data_len, (char*)evt->data);
    }
    return ESP_OK;
}

// 核心功能：HTTPS 请求
static void chat_with_doubao(const char* family_message) {
    if (family_message == NULL || strlen(family_message) == 0) return;

    ESP_LOGI(TAG, "\n========== 开始呼叫豆包大模型 ==========");
    ESP_LOGI(TAG, "家属留言: %s", family_message);

    esp_http_client_config_t config = {
        .url = "https://ark.cn-beijing.volces.com/api/v3/chat/completions",
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .event_handler = _http_event_handler,
        .buffer_size = 2048,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    
    char *auth_header = malloc(128);
    sprintf(auth_header, "Bearer %s", VOLCENGINE_API_KEY);
    esp_http_client_set_header(client, "Authorization", auth_header);

    char *post_data = malloc(1024);
    if (post_data) {
        snprintf(post_data, 1024,
            "{\"model\":\"%s\",\"messages\":["
            "{\"role\":\"system\",\"content\":\"你是陪伴老人的智能管家。家属发来留言，请用亲切、温暖、口语化的语气转述。要求不超过50字。\"},"
            "{\"role\":\"user\",\"content\":\"%s\"}]}",
            VOLCENGINE_ENDPOINT, family_message);
            
        esp_http_client_set_post_field(client, post_data, strlen(post_data));
        esp_err_t err = esp_http_client_perform(client);
        
        if (err == ESP_OK) ESP_LOGI(TAG, "\n========== 豆包回复完毕 ==========");
        else ESP_LOGE(TAG, "HTTP请求失败: %s", esp_err_to_name(err));
        
        free(post_data);
    }
    free(auth_header);
    esp_http_client_cleanup(client);
}

// 独立的 FreeRTOS 任务
static void doubao_chat_task(void *pvParameters) {
    chat_with_doubao((const char*)pvParameters);
    vTaskDelete(NULL); 
}

// 对外暴露的接口函数
void start_doubao_chat(const char* family_message) {
    xTaskCreate(doubao_chat_task, "doubao_task", 1024 * 12, (void *)family_message, 3, NULL);
}