#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h> 
#include "esp_log.h"
#include "mqtt_client.h"
#include "cJSON.h"
#include "doubao_ai.h" 
#include "voice_pipeline.h"
#include "music_screen.h"
#include "mqtt_app.h" // 确保包含了头文件

// 引入 FreeRTOS 队列相关头文件
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

static const char *TAG = "WKS_MQTT";
esp_mqtt_client_handle_t client = NULL;

// 1️⃣ 定义全局的 UI 消息队列句柄（快递驿站）
QueueHandle_t ui_msg_queue = NULL;

// 定义全局变量，暂存从微信小程序收到的数据
char global_elder_name[64] = {0};
char global_elder_status[128] = {0};
char global_family_msg[256] = {0};

static void mqtt_notification_chime_task(void *arg)
{
    (void)arg;
    if (music_screen_is_playing()) {
        music_screen_interrupt_playback("family_message");
    }
    voice_pipeline_play_notification_chime();
    vTaskDelete(NULL);
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "✅ 成功连上 EMQX 公网 MQTT 服务器！");
            esp_mqtt_client_subscribe(client, "wks_smart/screen/msg", 0);
            ESP_LOGI(TAG, "📡 已订阅频道: wks_smart/screen/msg，等待小程序发消息...");
            break;
            
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "❌ 与服务器失去连接，正在自动重连...");
            break;
            
        case MQTT_EVENT_DATA: 
            ESP_LOGI(TAG, "📥 叮咚！收到微信小程序发来的亲情数据！");
            
            // 1. 安全提取 MQTT 负载数据
            char *json_str = (char *)malloc(event->data_len + 1);
            if (json_str != NULL) {
                memcpy(json_str, event->data, event->data_len);
                json_str[event->data_len] = '\0'; 

                // 2. 召唤 cJSON 拆解数据包
                cJSON *root = cJSON_Parse(json_str);
                if (root) {
                    cJSON *name_item = cJSON_GetObjectItem(root, "name");
                    cJSON *status_item = cJSON_GetObjectItem(root, "status");
                    cJSON *msg_item = cJSON_GetObjectItem(root, "msg");
                    if (!msg_item) {
                        msg_item = cJSON_GetObjectItem(root, "message");
                    }

                    // 3. 校验并提取数据
                    if (name_item && status_item && (msg_item || status_item)) {
                        const char *note_text = NULL;
                        if (msg_item && cJSON_IsString(msg_item) && msg_item->valuestring && msg_item->valuestring[0] != '\0') {
                            note_text = msg_item->valuestring;
                        } else if (status_item && cJSON_IsString(status_item) && status_item->valuestring) {
                            note_text = status_item->valuestring;
                        } else {
                            note_text = "收到新的家庭动态";
                        }

                        // 存入全局变量
                        strncpy(global_elder_name, name_item->valuestring, sizeof(global_elder_name) - 1);
                        strncpy(global_elder_status, status_item->valuestring, sizeof(global_elder_status) - 1);
                        strncpy(global_family_msg, note_text, sizeof(global_family_msg) - 1);

                        ESP_LOGI(TAG, "🧑‍🦳 长辈称呼: %s", global_elder_name);
                        ESP_LOGI(TAG, "🏃 当前动态: %s", global_elder_status);
                        ESP_LOGI(TAG, "💌 亲情留言: %s", global_family_msg);
                        
                        // ==========================================
                        // 🚀 核心修改：打包消息，扔进 FreeRTOS 队列
                        // ==========================================
                        if (ui_msg_queue != NULL) {
                            ui_msg_t new_note;
                            snprintf(new_note.sender, sizeof(new_note.sender), "%s", name_item->valuestring);
                            snprintf(new_note.message, sizeof(new_note.message), "%s", note_text);
                            snprintf(new_note.time, sizeof(new_note.time), "刚刚"); // 默认写“刚刚”

                            // 投递到队列（非阻塞），让 LVGL 任务自己去取
                            xQueueSend(ui_msg_queue, &new_note, 0);
                            ESP_LOGI(TAG, "📦 亲情便签已塞入队列，等待 LVGL 屏幕画出！");
                        }

                        // 4. 将便签丢给豆包大模型去润色和语音播报
                        ESP_LOGI(TAG, "🚀 准备唤醒小柚子进行播报...");
                        xTaskCreate(mqtt_notification_chime_task,
                                    "mqtt_chime",
                                    4096,
                                    NULL,
                                    4,
                                    NULL);
                        start_doubao_chat(global_family_msg); 
                        
                    } else {
                        ESP_LOGW(TAG, "⚠️ JSON 格式不符，缺少 name/status 字段");
                    }
                    cJSON_Delete(root); // 释放 JSON 内存
                } else {
                    ESP_LOGE(TAG, "❌ JSON 解析失败，收到的格式不对: %s", json_str);
                }
                free(json_str); // 释放字符串内存
            }
            break;
            
        default:
            break;
    }
}

void mqtt_app_start(void) {
    // 2️⃣ 初始化 UI 消息队列 (最多存 5 条消息，防止爆内存)
    ui_msg_queue = xQueueCreate(5, sizeof(ui_msg_t));
    if (ui_msg_queue == NULL) {
        ESP_LOGE(TAG, "❌ UI 队列创建失败，内存不足！");
    }

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://broker.emqx.io", 
        .credentials.client_id = "ESP32_P4_XiaoYouZi_Screen", 
    };
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

// 给 face_ai.cpp 专属调用的发送函数
void sensehub_publish_message(const char* topic, const char* payload) {
    if (client == NULL) {
        ESP_LOGW(TAG, "MQTT 尚未连接，发送失败");
        return;
    }
    int msg_id = esp_mqtt_client_publish(client, topic, payload, 0, 1, 0);
    ESP_LOGI(TAG, "📤 AI 视觉联动：已发送数据包到频道 [%s], ID: %d", topic, msg_id);
}
