/**
 ******************************************************************************
 * @file        myi2s.h
 * @version     V1.0
 * @brief       I2S驱动代码（用于ES8311音频编解码器）
 ******************************************************************************
 */

#ifndef _MYI2S_H_
#define _MYI2S_H_

#include <stdio.h>
#include <string.h>
#include "driver/gpio.h"
#include "driver/i2s_std.h"

/* I2S 引脚定义 — 与触摸屏 I2C 引脚无冲突 */
#define I2S_NUM                 (I2S_NUM_0)                /* I2S port */
#define I2S_BCK_IO              (GPIO_NUM_4)               /* ES8311_SCLK */
#define I2S_WS_IO               (GPIO_NUM_6)               /* ES8311_LRCK */
#define I2S_DO_IO               (GPIO_NUM_3)               /* ES8311_SDIN  (ESP→ES8311, 播放) */
#define I2S_DI_IO               (GPIO_NUM_5)               /* ES8311_SDOUT (ES8311→ESP, 录音) */
#define I2S_MCK_IO              (GPIO_NUM_2)               /* ES8311_MCLK */

#define I2S_RECV_BUF_SIZE       (4096)                     /* 接收缓冲大小（字节）*/
#define I2S_SAMPLE_RATE         (16000)                    /* 采样率 16kHz（兼容STT）*/
#define I2S_MCLK_MULTIPLE       (384)                      /* MCLK倍数 */
#define EXAMPLE_MCLK_FREQ_HZ    (I2S_SAMPLE_RATE * I2S_MCLK_MULTIPLE)
#define EXAMPLE_VOICE_VOLUME    (60)                       /* 默认输出音量 */

extern i2s_chan_handle_t tx_handle;
extern i2s_chan_handle_t rx_handle;

/* 函数声明 */
esp_err_t myi2s_init(void);
void i2s_trx_start(void);
void i2s_trx_stop(void);
void i2s_deinit(void);
size_t i2s_tx_write(uint8_t *buffer, uint32_t frame_size);
size_t i2s_rx_read(uint8_t *buffer, uint32_t frame_size);
void i2s_set_samplerate_bits_sample(int samplerate, int bits_sample);

#endif /* _MYI2S_H_ */
