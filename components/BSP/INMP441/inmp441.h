/**
 ******************************************************************************
 * @file        inmp441.h
 * @version     V1.0
 * @brief       INMP441 I2S 数字麦克风驱动
 ******************************************************************************
 * @note        INMP441 通过 I2S_NUM_1 独立采集音频，不经过 ES8311
 *              引脚：SCK(位时钟) WS(字选择) SD(数据) + L/R接GND(左声道)
 ******************************************************************************
 */

#ifndef __INMP441_H__
#define __INMP441_H__

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>

/* ==================== INMP441 引脚定义 ==================== */
/* 用户根据实际接线修改这三个 GPIO */
#define INMP441_SCK_IO          (GPIO_NUM_7)    /* I2S1 位时钟 → INMP441 SCK */
#define INMP441_WS_IO           (GPIO_NUM_8)    /* I2S1 字选择 → INMP441 WS  */
#define INMP441_SD_IO           (GPIO_NUM_9)    /* I2S1 数据输入 ← INMP441 SD */

/* ==================== 采样参数 ==================== */
#define INMP441_SAMPLE_RATE     16000           /* 16kHz，兼容 ASR */
#define INMP441_BIT_WIDTH       16              /* 16-bit 输出（从24位截取高16位）*/

/**
 * @brief 初始化 INMP441 (I2S_NUM_1, 仅 RX)
 * @return ESP_OK 成功
 */
esp_err_t inmp441_init(void);

/**
 * @brief 从 INMP441 读取 PCM 音频数据（单声道 16bit）
 * @param buffer  输出缓冲区
 * @param len     要读取的字节数
 * @return 实际读取的字节数
 */
size_t inmp441_read(uint8_t *buffer, size_t len);

/**
 * @brief 卸载 INMP441
 */
void inmp441_deinit(void);

#endif /* __INMP441_H__ */
