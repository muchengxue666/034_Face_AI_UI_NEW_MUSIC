/**
 * @file    smoke_sensor.h
 * @brief   Flying Fish 烟雾传感器驱动（ADC 模拟输入）
 */
#ifndef __SMOKE_SENSOR_H__
#define __SMOKE_SENSOR_H__

#include "esp_err.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"
#include <stdbool.h>

/* ADC 引脚，根据实际接线修改 */
#define SMOKE_ADC_GPIO          GPIO_NUM_12
#define SMOKE_ADC_UNIT          ADC_UNIT_1
#define SMOKE_ADC_CHANNEL       ADC_CHANNEL_1    /* 需根据 GPIO 对应的 ADC 通道调整 */
#define SMOKE_ADC_ATTEN         ADC_ATTEN_DB_12  /* 0~3.3V 量程 */
#define SMOKE_ADC_BITWIDTH      ADC_BITWIDTH_12  /* 12位精度，0~4095 */

/* 烟雾报警阈值（ADC 原始值，需根据实际环境标定）*/
#define SMOKE_ALARM_THRESHOLD   3500

/**
 * @brief 初始化烟雾传感器 ADC
 */
esp_err_t smoke_sensor_init(void);

/**
 * @brief 读取烟雾浓度 ADC 原始值
 * @return ADC 值 0~4095，值越大浓度越高；-1=读取失败
 */
int smoke_sensor_read_raw(void);

/**
 * @brief 读取烟雾浓度电压值（mV）
 * @return 电压值（mV）；-1=读取失败
 */
int smoke_sensor_read_mv(void);

/**
 * @brief 判断是否触发烟雾报警
 * @return true=浓度超标
 */
bool smoke_sensor_is_alarm(void);

#endif
