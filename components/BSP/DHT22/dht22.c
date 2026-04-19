/**
 * @file    dht22.c
 * @brief   DHT22 温湿度传感器驱动（单线协议）
 * @note    DHT22 最小读取间隔 2 秒
 */
#include "dht22.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "rom/ets_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "DHT22";

esp_err_t dht22_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << DHT22_GPIO,
        .mode         = GPIO_MODE_INPUT_OUTPUT_OD,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
    gpio_set_level(DHT22_GPIO, 1);
    ESP_LOGI(TAG, "DHT22 initialized on GPIO%d", DHT22_GPIO);
    return ESP_OK;
}

/* 等待引脚变为指定电平，超时返回 -1，否则返回持续时间(us) */
static int wait_level(int level, int timeout_us)
{
    int64_t start = esp_timer_get_time();
    while (gpio_get_level(DHT22_GPIO) == level) {
        if ((esp_timer_get_time() - start) > timeout_us) return -1;
    }
    return (int)(esp_timer_get_time() - start);
}

static portMUX_TYPE s_dht_mux = portMUX_INITIALIZER_UNLOCKED;

esp_err_t dht22_read(dht22_data_t *data)
{
    if (!data) return ESP_ERR_INVALID_ARG;

    uint8_t bytes[5] = {0};
    esp_err_t ret = ESP_OK;

    /* 关中断保证时序精确 */
    taskENTER_CRITICAL(&s_dht_mux);

    /* 主机拉低 1ms 发送起始信号 */
    gpio_set_level(DHT22_GPIO, 0);
    ets_delay_us(1100);
    gpio_set_level(DHT22_GPIO, 1);
    ets_delay_us(30);

    /* 等待 DHT22 响应：低电平 80us + 高电平 80us */
    if (wait_level(0, 100) < 0) { ret = ESP_ERR_TIMEOUT; goto done; }
    if (wait_level(1, 100) < 0) { ret = ESP_ERR_TIMEOUT; goto done; }

    /* 读取 40 位数据 */
    for (int i = 0; i < 40; i++) {
        if (wait_level(0, 80) < 0) { ret = ESP_ERR_TIMEOUT; goto done; }
        int high_us = wait_level(1, 100);
        if (high_us < 0) { ret = ESP_ERR_TIMEOUT; goto done; }

        bytes[i / 8] <<= 1;
        if (high_us > 40) {
            bytes[i / 8] |= 1;  /* 高电平 > 40us 为 1 */
        }
    }

done:
    taskEXIT_CRITICAL(&s_dht_mux);

    if (ret != ESP_OK) return ret;

    /* 校验 */
    uint8_t checksum = bytes[0] + bytes[1] + bytes[2] + bytes[3];
    if (checksum != bytes[4]) {
        ESP_LOGW(TAG, "CRC failed: %02X+%02X+%02X+%02X=%02X, expected %02X",
                 bytes[0], bytes[1], bytes[2], bytes[3], checksum, bytes[4]);
        return ESP_ERR_INVALID_CRC;
    }

    /* 解析湿度和温度 */
    data->humidity    = ((bytes[0] << 8) | bytes[1]) * 0.1f;
    int16_t raw_temp  = ((bytes[2] & 0x7F) << 8) | bytes[3];
    data->temperature = raw_temp * 0.1f;
    if (bytes[2] & 0x80) data->temperature = -data->temperature;

    ESP_LOGI(TAG, "Temp=%.1f°C, Humidity=%.1f%%", data->temperature, data->humidity);
    return ESP_OK;
}
