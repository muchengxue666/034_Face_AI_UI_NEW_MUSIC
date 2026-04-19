#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_timer.h"

#include "led.h"
#include "myiic.h"
#include "mipi_cam.h"
#include "face_ai.h"
#include "gesture_ai.h"
#include "sdmmc.h"
#include "dht22.h"
#include "smoke_sensor.h"
#include "mqtt_app.h"
#include "wifi_app.h"
#include "doubao_ai.h"
#include "voice_pipeline.h"
#include "music_screen.h"
#include "lvgl_demo.h"

static const char *TAG = "MAIN";

#define SMOKE_ALERT_SAMPLE_MS          1500
#define SMOKE_ALERT_REPEAT_MS          8000
#define SMOKE_ALERT_BUSY_RETRY_MS      2000
#define SMOKE_ALERT_POST_VOICE_MS      5000
#define SMOKE_ALERT_FAIL_RETRY_MS      4000
#define SMOKE_ALARM_ASSERT_COUNT       2
#define SMOKE_ALARM_CLEAR_COUNT        3

static const char *SMOKE_ALERT_TEXT =
    u8"\u7237\u7237\uff0c\u68c0\u6d4b\u5230\u70df\u96fe\uff0c\u8bf7\u5c3d\u5feb\u524d\u5f80\u5b89\u5168\u533a\u57df\uff01";

static void smoke_alert_task(void *arg)
{
    (void)arg;

    int alarm_hits = 0;
    int clear_hits = 0;
    bool alarm_active = false;
    int64_t next_announce_ms = 0;
    int64_t last_voice_busy_ms = 0;

    while (1) {
        int64_t now_ms = esp_timer_get_time() / 1000;
        voice_state_t voice_state = voice_pipeline_get_state();
        if (voice_state != VOICE_STATE_IDLE) {
            last_voice_busy_ms = now_ms;
        }

        bool alarm = smoke_sensor_is_alarm();
        if (alarm) {
            alarm_hits++;
            clear_hits = 0;
        } else {
            clear_hits++;
            alarm_hits = 0;
        }

        if (!alarm_active && alarm_hits >= SMOKE_ALARM_ASSERT_COUNT) {
            alarm_active = true;
            next_announce_ms = 0;
            ESP_LOGW(TAG, "Smoke alert entered");
        } else if (alarm_active && clear_hits >= SMOKE_ALARM_CLEAR_COUNT) {
            alarm_active = false;
            next_announce_ms = 0;
            ESP_LOGI(TAG, "Smoke alert cleared");
        }

        if (alarm_active && (next_announce_ms == 0 || now_ms >= next_announce_ms)) {
            bool voice_busy = (voice_state != VOICE_STATE_IDLE);
            bool in_voice_cooldown = (last_voice_busy_ms != 0)
                                  && ((now_ms - last_voice_busy_ms) < SMOKE_ALERT_POST_VOICE_MS);

            if (voice_busy) {
                next_announce_ms = now_ms + SMOKE_ALERT_BUSY_RETRY_MS;
                ESP_LOGI(TAG, "Smoke alert deferred: voice busy (state=%d)", voice_state);
            } else if (in_voice_cooldown) {
                next_announce_ms = last_voice_busy_ms + SMOKE_ALERT_POST_VOICE_MS;
                ESP_LOGI(TAG, "Smoke alert deferred: waiting for voice cooldown (%lld ms left)",
                         (long long)(next_announce_ms - now_ms));
            } else {
                if (music_screen_is_playing()) {
                    music_screen_interrupt_playback("smoke_alert");
                }
                esp_err_t ret = voice_pipeline_speak_text(SMOKE_ALERT_TEXT);
                if (ret != ESP_OK) {
                    ESP_LOGW(TAG, "Smoke alert TTS failed: %s", esp_err_to_name(ret));
                    next_announce_ms = now_ms + SMOKE_ALERT_FAIL_RETRY_MS;
                } else {
                    next_announce_ms = now_ms + SMOKE_ALERT_REPEAT_MS;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(SMOKE_ALERT_SAMPLE_MS));
    }
}

static void app_services_task(void *arg)
{
    (void)arg;

    ESP_LOGI(TAG, "Connecting WiFi...");
    wifi_sta_init();

    ESP_LOGI(TAG, "Starting MQTT...");
    mqtt_app_start();

    ESP_LOGI(TAG, "Starting AI engine...");
    start_doubao_chat("Device started, waiting for MQTT messages.");

    ESP_LOGI(TAG, "Starting voice pipeline...");
    esp_err_t ret = voice_pipeline_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Voice pipeline init failed: %s (continuing)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Starting smoke alert monitor...");
        xTaskCreatePinnedToCore(smoke_alert_task, "smoke_alert", 6144, NULL, 5, NULL, 1);
    }

    ESP_LOGI(TAG, "Loading gesture recognition model...");
    init_gesture_ai();

    vTaskDelete(NULL);
}

static void spiffs_init(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "storage",
        .max_files = 8,
        .format_if_mount_failed = false,
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "SPIFFS mount failed (%s), GIF will not load", esp_err_to_name(ret));
    } else {
        size_t total = 0;
        size_t used = 0;
        esp_spiffs_info("storage", &total, &used);
        ESP_LOGI(TAG, "SPIFFS mounted: %d/%d bytes used", used, total);
    }
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    ret = esp_netif_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(ret);
    }

    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(ret);
    }

    led_init();
    dht22_init();
    smoke_sensor_init();

    {
        int raw = smoke_sensor_read_raw();
        int mv = smoke_sensor_read_mv();
        ESP_LOGI(TAG, "Smoke sensor test: raw=%d, mv=%d", raw, mv);

        dht22_data_t dht;
        esp_err_t dht_ret = dht22_read(&dht);
        if (dht_ret == ESP_OK) {
            ESP_LOGI(TAG, "DHT22 test: temp=%.1f, humi=%.1f%%", dht.temperature, dht.humidity);
        } else {
            ESP_LOGW(TAG, "DHT22 test: read failed (%s)", esp_err_to_name(dht_ret));
        }
    }

    myiic_init();
    mipi_cam_init();
    init_face_ai();

    xTaskCreatePinnedToCore(app_services_task, "app_services", 16384, NULL, 5, NULL, 0);

    ESP_LOGI(TAG, "Starting LVGL UI...");
    spiffs_init();
    lvgl_demo();
}
