/**
 * @file    smoke_sensor.c
 * @brief   Flying Fish 烟雾传感器驱动（ADC 模拟输入）
 * @note    MQ-2 类传感器，模拟输出，VCC 接 5V，AO 接 ESP32 ADC 引脚
 *          加热器需要预热 2-3 分钟后数据才稳定
 */
#include "smoke_sensor.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

static const char *TAG = "SMOKE";

static adc_oneshot_unit_handle_t s_adc_handle = NULL;
static adc_cali_handle_t s_cali_handle = NULL;
static bool s_cali_ok = false;

esp_err_t smoke_sensor_init(void)
{
    /* 创建 ADC oneshot 单元 */
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = SMOKE_ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &s_adc_handle));

    /* 配置 ADC 通道 */
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten    = SMOKE_ADC_ATTEN,
        .bitwidth = SMOKE_ADC_BITWIDTH,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc_handle, SMOKE_ADC_CHANNEL, &chan_cfg));

    /* 尝试创建校准句柄（线性拟合） */
#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t cali_cfg = {
        .unit_id  = SMOKE_ADC_UNIT,
        .atten    = SMOKE_ADC_ATTEN,
        .bitwidth = SMOKE_ADC_BITWIDTH,
    };
    if (adc_cali_create_scheme_line_fitting(&cali_cfg, &s_cali_handle) == ESP_OK) {
        s_cali_ok = true;
    }
#elif ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id  = SMOKE_ADC_UNIT,
        .atten    = SMOKE_ADC_ATTEN,
        .bitwidth = SMOKE_ADC_BITWIDTH,
        .chan     = SMOKE_ADC_CHANNEL,
    };
    if (adc_cali_create_scheme_curve_fitting(&cali_cfg, &s_cali_handle) == ESP_OK) {
        s_cali_ok = true;
    }
#endif

    ESP_LOGI(TAG, "Smoke sensor initialized on ADC%d_CH%d (GPIO%d), calibration=%s",
             SMOKE_ADC_UNIT, SMOKE_ADC_CHANNEL, SMOKE_ADC_GPIO,
             s_cali_ok ? "OK" : "not available");
    return ESP_OK;
}

int smoke_sensor_read_raw(void)
{
    if (!s_adc_handle) return -1;
    int raw = 0;
    if (adc_oneshot_read(s_adc_handle, SMOKE_ADC_CHANNEL, &raw) != ESP_OK) {
        return -1;
    }
    return raw;
}

int smoke_sensor_read_mv(void)
{
    if (!s_adc_handle) return -1;
    int raw = smoke_sensor_read_raw();
    if (raw < 0) return -1;

    if (s_cali_ok && s_cali_handle) {
        int mv = 0;
        if (adc_cali_raw_to_voltage(s_cali_handle, raw, &mv) == ESP_OK) {
            return mv;
        }
    }
    /* 无校准时粗略换算：3300mV / 4095 */
    return (raw * 3300) / 4095;
}

bool smoke_sensor_is_alarm(void)
{
    int raw = smoke_sensor_read_raw();
    if (raw < 0) return false;
    bool alarm = (raw > SMOKE_ALARM_THRESHOLD);
    if (alarm) {
        ESP_LOGW(TAG, "SMOKE ALARM! ADC=%d (threshold=%d)", raw, SMOKE_ALARM_THRESHOLD);
    }
    return alarm;
}
