/**
 * @file    dht22.h
 * @brief   DHT22 温湿度传感器驱动（单线协议）
 */
#ifndef __DHT22_H__
#define __DHT22_H__

#include "esp_err.h"
#include "driver/gpio.h"

/* 数据引脚，根据实际接线修改 */
#define DHT22_GPIO          GPIO_NUM_10

/**
 * @brief DHT22 读取结果
 */
typedef struct {
    float temperature;  /* 温度（°C）*/
    float humidity;     /* 湿度（%RH）*/
} dht22_data_t;

/**
 * @brief 初始化 DHT22
 */
esp_err_t dht22_init(void);

/**
 * @brief 读取温湿度（阻塞，约 4ms）
 * @param data 输出结果
 * @return ESP_OK=成功, ESP_ERR_TIMEOUT=无响应, ESP_ERR_INVALID_CRC=校验失败
 */
esp_err_t dht22_read(dht22_data_t *data);

#endif
