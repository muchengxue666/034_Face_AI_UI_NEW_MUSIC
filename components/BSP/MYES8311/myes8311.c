/**
 ******************************************************************************
 * @file        myes8311.c
 * @version     V1.0
 * @brief       ES8311音频编解码器驱动
 ******************************************************************************
 */

#include "myes8311.h"

static const char *TAG = "ES8311";

esp_codec_dev_handle_t g_codec_dev = NULL;  /* 全局编解码器句柄 */

/**
 * @brief   ES8311初始化
 * @note    共享 my_i2c_bus_handle (I2C_NUM_0, GPIO28/29)，与摄像头共用
 *          录音由 INMP441 (I2S_NUM_1) 负责，ES8311 只管播放
 */
esp_err_t myes8311_init(void)
{
    /* 直接使用 myiic 已创建的 I2C 总线（I2C_NUM_0, GPIO28/29）*/
    audio_codec_i2c_cfg_t i2c_cfg = {
        .port       = IIC_NUM_PORT,
        .addr       = ES8311_CODEC_DEFAULT_ADDR,
        .bus_handle = my_i2c_bus_handle,
    };
    const audio_codec_ctrl_if_t *ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    if (!ctrl_if) {
        ESP_LOGE(TAG, "Failed to create I2C ctrl interface");
        return ESP_FAIL;
    }

    /* 创建 I2S 数据接口 */
    audio_codec_i2s_cfg_t i2s_cfg = {
        .port      = I2S_NUM,
        .rx_handle = rx_handle,
        .tx_handle = tx_handle,
    };
    const audio_codec_data_if_t *data_if = audio_codec_new_i2s_data(&i2s_cfg);
    if (!data_if) {
        ESP_LOGE(TAG, "Failed to create I2S data interface");
        return ESP_FAIL;
    }

    /* 创建 GPIO 接口（用于PA控制）*/
    const audio_codec_gpio_if_t *gpio_if = audio_codec_new_gpio();
    if (!gpio_if) {
        ESP_LOGE(TAG, "Failed to create GPIO interface");
        return ESP_FAIL;
    }

    /* 配置 ES8311 编解码器 */
    es8311_codec_cfg_t es8311_cfg = {
        .ctrl_if    = ctrl_if,
        .gpio_if    = gpio_if,
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC,  /* 仅播放，录音由 INMP441 负责 */
        .master_mode = false,
        .use_mclk   = (I2S_MCK_IO >= 0),
        .pa_pin     = PA_CTRL_GPIO_PIN,
        .pa_reverted = false,
        .hw_gain    = {
            .pa_voltage      = 5.0,
            .codec_dac_voltage = 3.3,
        },
        .mclk_div = I2S_MCLK_MULTIPLE,
    };
    const audio_codec_if_t *es8311_if = es8311_codec_new(&es8311_cfg);
    if (!es8311_if) {
        ESP_LOGE(TAG, "Failed to create ES8311 interface");
        return ESP_FAIL;
    }

    /* 创建顶层编解码器设备 */
    esp_codec_dev_cfg_t dev_cfg = {
        .dev_type  = ESP_CODEC_DEV_TYPE_IN_OUT,
        .codec_if  = es8311_if,
        .data_if   = data_if,
    };
    g_codec_dev = esp_codec_dev_new(&dev_cfg);
    if (!g_codec_dev) {
        ESP_LOGE(TAG, "Failed to create codec device");
        return ESP_FAIL;
    }

    /* 打开设备并设置采样参数 */
    esp_codec_dev_sample_info_t sample_cfg = {
        .bits_per_sample = I2S_DATA_BIT_WIDTH_16BIT,
        .channel         = 2,
        .channel_mask    = 0x03,
        .sample_rate     = I2S_SAMPLE_RATE,
    };
    if (esp_codec_dev_open(g_codec_dev, &sample_cfg) != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "Failed to open codec device");
        return ESP_FAIL;
    }

    /* 设置默认音量 */
    if (esp_codec_dev_set_out_vol(g_codec_dev, EXAMPLE_VOICE_VOLUME) != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "Failed to set output volume");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "ES8311 initialized (DAC only), volume=%d", EXAMPLE_VOICE_VOLUME);
    return ESP_OK;
}

esp_err_t myes8311_set_volume(int volume)
{
    if (!g_codec_dev) return ESP_ERR_INVALID_STATE;
    return (esp_codec_dev_set_out_vol(g_codec_dev, volume) == ESP_CODEC_DEV_OK) ? ESP_OK : ESP_FAIL;
}

esp_codec_dev_handle_t myes8311_get_codec(void)
{
    return g_codec_dev;
}
