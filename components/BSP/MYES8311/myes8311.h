/**
 ******************************************************************************
 * @file        myes8311.h
 * @version     V1.0
 * @brief       ES8311音频编解码器驱动
 ******************************************************************************
 * @note        I2C总线冲突规避：
 *              - 触摸屏 GT9xxx 使用 I2C_NUM_0 (GPIO16/GPIO17)
 *              - ES8311 使用 I2C_NUM_1 (GPIO28/GPIO29)，互不冲突
 ******************************************************************************
 */

#ifndef __MYES8311_H_
#define __MYES8311_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "math.h"
#include "string.h"
#include "es8311.h"
#include "myi2s.h"
#include "myiic.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_codec_dev_defaults.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_vol.h"

/* 引脚定义 */
#define PA_CTRL_GPIO_PIN          GPIO_NUM_11               /* 功放使能引脚 */
#define ES8311_IIC_NUM_PORT       I2C_NUM_0                 /* 与摄像头共用 I2C_NUM_0 (GPIO28/29) */
#define ES8311_IIC_SDA_GPIO_PIN   GPIO_NUM_28               /* I2C0_SDA */
#define ES8311_IIC_SCL_GPIO_PIN   GPIO_NUM_29               /* I2C0_SCL */

/* 外部句柄（供语音模块使用）*/
extern esp_codec_dev_handle_t g_codec_dev;

/* 函数声明 */
esp_err_t myes8311_init(void);
esp_err_t myes8311_set_volume(int volume);
esp_codec_dev_handle_t myes8311_get_codec(void);

#endif /* __MYES8311_H_ */
