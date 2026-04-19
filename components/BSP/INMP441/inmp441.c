/**
 ******************************************************************************
 * @file        inmp441.c
 * @version     V1.0
 * @brief       INMP441 I2S 数字麦克风驱动
 ******************************************************************************
 * @note        INMP441 输出 I2S Philips 格式，24-bit 数据（MSB 对齐在 32-bit 帧内）
 *              L/R 接 GND → 数据在左声道(WS=0)
 *              本驱动使用 I2S_NUM_1 的 RX 通道，不影响 ES8311 的 I2S_NUM_0
 ******************************************************************************
 */

#include "inmp441.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "INMP441";

static i2s_chan_handle_t s_rx_handle = NULL;

#define INMP441_BASE_SHIFT        11
#define INMP441_MAX_SHIFT         16
#define INMP441_TARGET_PEAK_16BIT 24000

esp_err_t inmp441_init(void)
{
    ESP_LOGI(TAG, "Initializing INMP441 on I2S_NUM_1 (SCK=%d, WS=%d, SD=%d)",
             INMP441_SCK_IO, INMP441_WS_IO, INMP441_SD_IO);

    /* 创建 I2S 通道（仅 RX）*/
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &s_rx_handle));  /* NULL=不需要TX */

    /* 标准 I2S 配置（Philips 格式，与 INMP441 匹配）
     * INMP441 输出 24-bit 数据在 32-bit 帧内，用 32bit 读取后右移8位得到有效值 */
    i2s_std_config_t std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = INMP441_SAMPLE_RATE,
            .clk_src        = I2S_CLK_SRC_DEFAULT,
            .mclk_multiple  = I2S_MCLK_MULTIPLE_256,
        },
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = INMP441_SCK_IO,
            .ws   = INMP441_WS_IO,
            .dout = I2S_GPIO_UNUSED,
            .din  = INMP441_SD_IO,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };

    /* 调试：读取立体声，观察左右声道原始数据 */
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_BOTH;

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(s_rx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(s_rx_handle));

    ESP_LOGI(TAG, "INMP441 initialized: %dHz, %d-bit, mono",
             INMP441_SAMPLE_RATE, INMP441_BIT_WIDTH);
    return ESP_OK;
}

size_t inmp441_read(uint8_t *buffer, size_t len)
{
    if (!s_rx_handle || !buffer || len == 0) return 0;

    /* 立体声 32bit: 每个单声道 16bit 采样需要 8 字节 I2S 数据 (L32+R32) */
    size_t mono_samples = len / 2;
    size_t i2s_read_len = mono_samples * 8;  /* 每采样 = 4字节L + 4字节R */
    uint8_t *tmp_buf = malloc(i2s_read_len);
    if (!tmp_buf) return 0;

    size_t bytes_read = 0;
    esp_err_t ret = i2s_channel_read(s_rx_handle, tmp_buf, i2s_read_len, &bytes_read, 1000);
    if (ret != ESP_OK || bytes_read == 0) {
        free(tmp_buf);
        return 0;
    }

    /* 32bit 立体声 → 16bit 单声道
     * INMP441 输出 24bit 数据左对齐在 32bit 内（bits[31:8]有效）
     * >> 11 = 保留 24bit 中的高 13 位，映射到 int16_t 范围
     * 比 >> 16 多保留 5 位精度，信号放大 32 倍 */
    int32_t *src = (int32_t*)tmp_buf;
    int16_t *dst = (int16_t*)buffer;
    size_t samples = bytes_read / 8;  /* 立体声32bit: 每个采样8字节 */
    static bool first_print = true;
    static uint32_t s_shift_log_count = 0;
    if (first_print && samples >= 4) {
        ESP_LOGI(TAG, "RAW32 L: %ld %ld %ld %ld", src[0], src[2], src[4], src[6]);
        ESP_LOGI(TAG, "RAW32 R: %ld %ld %ld %ld", src[1], src[3], src[5], src[7]);
        first_print = false;
    }
    int32_t peak_raw = 0;
    for (size_t i = 0; i < samples; i++) {
        int32_t raw = src[i * 2];
        int32_t abs_raw = raw >= 0 ? raw : -raw;
        if (abs_raw > peak_raw) {
            peak_raw = abs_raw;
        }
    }

    int shift = INMP441_BASE_SHIFT;
    while (shift < INMP441_MAX_SHIFT && (peak_raw >> shift) > INMP441_TARGET_PEAK_16BIT) {
        shift++;
    }
    if (shift > INMP441_BASE_SHIFT && s_shift_log_count < 5) {
        ESP_LOGW(TAG, "Capture peak high, auto-attenuating chunk: raw_peak=%ld shift=%d",
                 (long)peak_raw, shift);
        s_shift_log_count++;
    }
    for (size_t i = 0; i < samples; i++) {
        int32_t val = src[i * 2] >> shift;  /* 取左声道，按当前块峰值自适应防削顶 */
        if (val > 32767) val = 32767;
        if (val < -32768) val = -32768;
        dst[i] = (int16_t)val;
    }

    free(tmp_buf);
    return samples * 2;  /* 返回 16bit 字节数 */
}

void inmp441_deinit(void)
{
    if (s_rx_handle) {
        i2s_channel_disable(s_rx_handle);
        i2s_del_channel(s_rx_handle);
        s_rx_handle = NULL;
        ESP_LOGI(TAG, "INMP441 deinitialized");
    }
}
