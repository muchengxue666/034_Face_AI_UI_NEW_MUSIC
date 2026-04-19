/**
 ******************************************************************************
 * @file        myi2s.c
 * @version     V1.0
 * @brief       I2S驱动代码（用于ES8311音频编解码器）
 ******************************************************************************
 */

#include "myi2s.h"
#include "esp_err.h"
#include "esp_log.h"

i2s_chan_handle_t tx_handle = NULL;
i2s_chan_handle_t rx_handle = NULL;
static i2s_std_config_t my_std_cfg;
static const char *TAG = "MYI2S";

esp_err_t myi2s_init(void)
{
    if (tx_handle && rx_handle) {
        ESP_LOGI(TAG, "I2S channels already initialized");
        return ESP_OK;
    }

    esp_err_t ret;
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    ret = i2s_new_channel(&chan_cfg, &tx_handle, &rx_handle);
    if (ret != ESP_OK) {
        tx_handle = NULL;
        rx_handle = NULL;
        ESP_LOGW(TAG, "i2s_new_channel failed: %s", esp_err_to_name(ret));
        return ret;
    }

    i2s_std_config_t std_cfg = {
        .clk_cfg  = {
            .sample_rate_hz = I2S_SAMPLE_RATE,
            .clk_src        = I2S_CLK_SRC_DEFAULT,
            .mclk_multiple  = I2S_MCLK_MULTIPLE,
        },
        .slot_cfg = {
            .data_bit_width = I2S_DATA_BIT_WIDTH_16BIT,
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
            .slot_mode      = I2S_SLOT_MODE_STEREO,
            .slot_mask      = I2S_STD_SLOT_BOTH,
            .ws_width       = I2S_DATA_BIT_WIDTH_16BIT,
            .ws_pol         = false,
            .bit_shift      = true,
            .left_align     = false,
            .big_endian     = false,
            .bit_order_lsb  = false,
        },
        .gpio_cfg = {
            .mclk = I2S_MCK_IO,
            .bclk = I2S_BCK_IO,
            .ws   = I2S_WS_IO,
            .dout = I2S_DO_IO,
            .din  = I2S_DI_IO,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };

    my_std_cfg = std_cfg;

    ret = i2s_channel_init_std_mode(tx_handle, &std_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "init tx std mode failed: %s", esp_err_to_name(ret));
        goto fail;
    }

    ret = i2s_channel_init_std_mode(rx_handle, &std_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "init rx std mode failed: %s", esp_err_to_name(ret));
        goto fail;
    }

    ret = i2s_channel_enable(tx_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "enable tx channel failed: %s", esp_err_to_name(ret));
        goto fail;
    }

    ret = i2s_channel_enable(rx_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "enable rx channel failed: %s", esp_err_to_name(ret));
        goto fail;
    }

    return ESP_OK;

fail:
    if (tx_handle) {
        i2s_del_channel(tx_handle);
        tx_handle = NULL;
    }
    if (rx_handle) {
        i2s_del_channel(rx_handle);
        rx_handle = NULL;
    }
    return ret;
}

void i2s_trx_start(void)
{
    if (tx_handle) i2s_channel_enable(tx_handle);
    if (rx_handle) i2s_channel_enable(rx_handle);
}

void i2s_trx_stop(void)
{
    if (tx_handle) i2s_channel_disable(tx_handle);
    if (rx_handle) i2s_channel_disable(rx_handle);
}

void i2s_deinit(void)
{
    if (tx_handle) {
        i2s_channel_disable(tx_handle);
        i2s_del_channel(tx_handle);
        tx_handle = NULL;
    }
    if (rx_handle) {
        i2s_channel_disable(rx_handle);
        i2s_del_channel(rx_handle);
        rx_handle = NULL;
    }
}

void i2s_set_samplerate_bits_sample(int samplerate, int bits_sample)
{
    if (!tx_handle) {
        return;
    }
    i2s_channel_disable(tx_handle);
    my_std_cfg.slot_cfg.ws_width = bits_sample;
    i2s_channel_reconfig_std_slot(tx_handle, &my_std_cfg.slot_cfg);
    my_std_cfg.clk_cfg.sample_rate_hz = samplerate;
    i2s_channel_reconfig_std_clock(tx_handle, &my_std_cfg.clk_cfg);
    i2s_channel_enable(tx_handle);
}

size_t i2s_tx_write(uint8_t *buffer, uint32_t frame_size)
{
    if (!tx_handle || !buffer || frame_size == 0) {
        return 0;
    }
    size_t bytes_written;
    i2s_channel_write(tx_handle, buffer, frame_size, &bytes_written, 1000);
    return bytes_written;
}

size_t i2s_rx_read(uint8_t *buffer, uint32_t frame_size)
{
    if (!rx_handle || !buffer || frame_size == 0) {
        return 0;
    }
    size_t bytes_read;
    i2s_channel_read(rx_handle, buffer, frame_size, &bytes_read, 1000);
    return bytes_read;
}
