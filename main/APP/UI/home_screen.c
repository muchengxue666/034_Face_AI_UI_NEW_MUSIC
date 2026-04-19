/**
 ******************************************************************************
 * @file        home_screen.c
 * @version     V4.0
 * @brief       老黄历主界面 - 实时时间 + 农历 + 大字体适老化
 ******************************************************************************
 */

#include "home_screen.h"
#include "ai_assistant_ui.h"
#include "ui_theme.h"
#include "voice_pipeline.h"
#include "esp_lvgl_port.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "cJSON.h"
#include "wifi_app.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <zlib.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

static const char *TAG = "home_screen";

/* ==================== 颜色定义 ==================== */
#define ALMANAC_PAPER_BG        THEME_CARD_BG
#define ALMANAC_PAPER_BORDER    lv_color_hex(0xD4B896)
#define ALMANAC_RED             THEME_ACCENT_RED
#define ALMANAC_GOLD            THEME_ACCENT_GOLD
#define ALMANAC_TEXT_BLACK      THEME_TEXT_PRIMARY
#define ALMANAC_TEXT_GRAY       THEME_TEXT_SECONDARY

/* ==================== 屏幕对象 ==================== */
static lv_obj_t *s_home_screen       = NULL;
static lv_obj_t *s_almanac_container = NULL;
static lv_obj_t *s_left_panel        = NULL;
static lv_obj_t *s_right_panel       = NULL;

/* 天气标签（后台任务更新用）*/
static lv_obj_t *s_weather_icon_lbl  = NULL;   /* 天气图标文字：晴/阴/雨 */
static lv_obj_t *s_weather_temp_lbl  = NULL;   /* 温度 */
static lv_obj_t *s_weather_desc_lbl  = NULL;   /* 天气描述 */
static lv_obj_t *s_weather_hum_lbl   = NULL;   /* 湿度 */

/* 需要实时刷新的标签 */
static lv_obj_t *s_label_lunar_year   = NULL;
static lv_obj_t *s_label_lunar_md     = NULL;
static lv_obj_t *s_label_solar        = NULL;
static lv_obj_t *s_label_solar_term   = NULL;
static lv_obj_t *s_label_time         = NULL;   /* 时:分 */

/* 刷新定时器 */
static lv_timer_t *s_time_timer = NULL;

/* ==================== 天气配置 ==================== */
/* 和风天气免费API：https://dev.qweather.com 注册后获取key */
#define WEATHER_API_KEY     "e40450289d87452f92fdd7caa7c988ca"   /* ← 填你的和风 key */
#define WEATHER_LOCATION    "101190101"           /* 城市ID：南京 */
#define WEATHER_URL         "https://n64ewu43qn.re.qweatherapi.com/v7/weather/now?location=" \
                            WEATHER_LOCATION "&key=" WEATHER_API_KEY
#define WEATHER_BUF_SIZE    2048
#define WEATHER_INTERVAL_MS (30 * 60 * 1000)      /* 每30分钟刷新 */

static char s_weather_buf[WEATHER_BUF_SIZE];
static int  s_weather_buf_len = 0;

/* Gzip解压：和风接口有时会强制返回gzip，需要先解压再做JSON解析 */
static bool weather_gzip_decompress(const uint8_t *in, size_t in_len, char **out, size_t *out_len)
{
    if (!in || in_len == 0 || !out || !out_len) {
        return false;
    }

    z_stream zs;
    memset(&zs, 0, sizeof(zs));

    int zret = inflateInit2(&zs, 16 + MAX_WBITS); /* 16+MAX_WBITS 表示gzip流 */
    if (zret != Z_OK) {
        ESP_LOGW(TAG, "Weather gzip init failed: %d", zret);
        return false;
    }

    size_t cap = 4096;
    char *buf = (char *)malloc(cap + 1);
    if (!buf) {
        inflateEnd(&zs);
        return false;
    }

    zs.next_in = (Bytef *)in;
    zs.avail_in = (uInt)in_len;

    while (1) {
        if (zs.total_out >= cap) {
            size_t new_cap = cap * 2;
            char *new_buf = (char *)realloc(buf, new_cap + 1);
            if (!new_buf) {
                free(buf);
                inflateEnd(&zs);
                return false;
            }
            buf = new_buf;
            cap = new_cap;
        }

        zs.next_out = (Bytef *)(buf + zs.total_out);
        zs.avail_out = (uInt)(cap - zs.total_out);

        zret = inflate(&zs, Z_NO_FLUSH);
        if (zret == Z_STREAM_END) {
            break;
        }
        if (zret != Z_OK) {
            ESP_LOGW(TAG, "Weather gzip inflate failed: %d", zret);
            free(buf);
            inflateEnd(&zs);
            return false;
        }
    }

    *out_len = zs.total_out;
    buf[*out_len] = '\0';
    *out = buf;

    inflateEnd(&zs);
    return true;
}

/* 天气code → 简短图标文字 */
static const char* weather_code_to_icon(int code)
{
    if (code == 100) return "晴";
    if (code >= 101 && code <= 104) return "云";
    if (code >= 300 && code <= 399) return "雨";
    if (code >= 400 && code <= 499) return "雪";
    if (code == 500 || code == 501) return "雾";
    return "—";
}

static esp_err_t weather_http_event(esp_http_client_event_t *evt)
{
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        int remaining = WEATHER_BUF_SIZE - s_weather_buf_len - 1;
        if (remaining > 0) {
            int copy = evt->data_len < remaining ? evt->data_len : remaining;
            memcpy(s_weather_buf + s_weather_buf_len, evt->data, copy);
            s_weather_buf_len += copy;
            s_weather_buf[s_weather_buf_len] = '\0';
        }
    }
    return ESP_OK;
}

static void weather_fetch_task(void *arg)
{
    /* 轮询等待WiFi真正连上，每秒检查一次，最多等60秒 */
    int wait = 0;
    while (!wifi_is_connected() && wait < 60) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        wait++;
    }
    if (!wifi_is_connected()) {
        ESP_LOGW(TAG, "Weather: WiFi not connected after 60s, giving up");
        vTaskDelete(NULL);
        return;
    }
    /* WiFi刚连上，稳定2秒再请求 */
    vTaskDelay(pdMS_TO_TICKS(2000));

    while (1) {
        s_weather_buf_len = 0;
        memset(s_weather_buf, 0, sizeof(s_weather_buf));

        esp_http_client_config_t cfg = {
            .url            = WEATHER_URL,
            .event_handler  = weather_http_event,
            .crt_bundle_attach = esp_crt_bundle_attach,
            .timeout_ms     = 8000,
        };
        esp_http_client_handle_t client = esp_http_client_init(&cfg);
        /* 强制请求明文JSON，避免服务端返回gzip后本地直接按字符串解析失败 */
        esp_http_client_set_header(client, "Accept-Encoding", "identity");
        esp_err_t err = esp_http_client_perform(client);
        int http_status = esp_http_client_get_status_code(client);
        int content_length = esp_http_client_get_content_length(client);
        esp_http_client_cleanup(client);

        if (err == ESP_OK && http_status == 200 && s_weather_buf_len > 0) {
            const char *json_start = s_weather_buf;
            char *json_decoded = NULL;
            size_t json_decoded_len = 0;

            if ((unsigned char)s_weather_buf[0] == 0x1F &&
                (unsigned char)s_weather_buf[1] == 0x8B) {
                if (weather_gzip_decompress((const uint8_t *)s_weather_buf,
                                            (size_t)s_weather_buf_len,
                                            &json_decoded,
                                            &json_decoded_len)) {
                    json_start = json_decoded;
                    ESP_LOGI(TAG, "Weather gzip decompressed: %d -> %d bytes",
                             s_weather_buf_len, (int)json_decoded_len);
                } else {
                    ESP_LOGW(TAG, "Weather gzip decompress failed, keep raw payload");
                }
            }

            /* 跳过UTF-8 BOM和前导空白 */
            if ((unsigned char)json_start[0] == 0xEF &&
                (unsigned char)json_start[1] == 0xBB &&
                (unsigned char)json_start[2] == 0xBF) {
                json_start += 3;
            }
            while (*json_start == ' ' || *json_start == '\r' || *json_start == '\n' || *json_start == '\t') {
                json_start++;
            }

            cJSON *root = cJSON_Parse(json_start);
            if (root) {
                cJSON *now = cJSON_GetObjectItem(root, "now");
                if (now) {
                    cJSON *code_item = cJSON_GetObjectItem(now, "icon");
                    cJSON *temp_item = cJSON_GetObjectItem(now, "temp");
                    cJSON *text_item = cJSON_GetObjectItem(now, "text");
                    cJSON *hum_item = cJSON_GetObjectItem(now, "humidity");

                    int code = (cJSON_IsString(code_item) && code_item->valuestring) ? atoi(code_item->valuestring) : 0;
                    const char *temp = (cJSON_IsString(temp_item) && temp_item->valuestring) ? temp_item->valuestring : "--";
                    const char *text = (cJSON_IsString(text_item) && text_item->valuestring) ? text_item->valuestring : "未知";
                    const char *hum = (cJSON_IsString(hum_item) && hum_item->valuestring) ? hum_item->valuestring : "--";

                    /* 在LVGL锁内更新UI */
                    if (lvgl_port_lock(100)) {
                        if (s_weather_icon_lbl) lv_label_set_text(s_weather_icon_lbl, weather_code_to_icon(code));
                        if (s_weather_temp_lbl) lv_label_set_text_fmt(s_weather_temp_lbl, "%s度", temp);
                        if (s_weather_desc_lbl) lv_label_set_text(s_weather_desc_lbl, text);
                        if (s_weather_hum_lbl)  lv_label_set_text_fmt(s_weather_hum_lbl, "湿度 %s%%", hum);
                        lvgl_port_unlock();
                    }
                    ESP_LOGI(TAG, "Weather: %s %s°C %s 湿度%s%%", weather_code_to_icon(code), temp, text, hum);
                } else {
                    ESP_LOGW(TAG, "Weather parse failed: missing now");
                }
                cJSON_Delete(root);
            } else {
                const char *err_ptr = cJSON_GetErrorPtr();
                unsigned char b0 = (unsigned char)s_weather_buf[0];
                unsigned char b1 = (unsigned char)s_weather_buf[1];
                unsigned char b2 = (unsigned char)s_weather_buf[2];
                ESP_LOGW(TAG, "Weather parse failed: invalid JSON, len=%d, content_len=%d, head=%02X %02X %02X",
                         s_weather_buf_len, content_length, b0, b1, b2);
                if (err_ptr) {
                    ESP_LOGW(TAG, "Weather parse error near: %.48s", err_ptr);
                }
                if (b0 == 0x1F && b1 == 0x8B) {
                    ESP_LOGW(TAG, "Weather payload looks like gzip data");
                }
            }

            if (json_decoded) {
                free(json_decoded);
            }
        } else {
            ESP_LOGW(TAG, "Weather fetch failed: err=%s, http=%d", esp_err_to_name(err), http_status);
            if (s_weather_buf_len > 0) {
                ESP_LOGW(TAG, "Weather response: %s", s_weather_buf);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(WEATHER_INTERVAL_MS));
    }
}


/* 农历数据表 1900-2100，每条 3 字节：
   高20bit = 12个月大小月(1=大30天,0=小29天)，
   次4bit = 闰月月份(0=无)，
   低8bit = 春节公历日(1月=0x0?，2月=0x1?) 用偏移法：
   实际用简化表，格式参考寿星万年历思路 */

/* 简化农历表（1900-2100），每年4字节：
   [0]=正月初一公历月日高字节(月)
   [1]=正月初一公历日
   [2-3]=12个月大小月位(bit11..bit0 = 一月..十二月, 1=大30, 0=小29) + 高4bit=闰月 */
static const uint8_t s_lunar_table[][4] = {
    /* 2020 */ {1,25, 0xA5,0x35},
    /* 2021 */ {2,12, 0xD4,0xA5},
    /* 2022 */ {2, 1, 0xA5,0x25},
    /* 2023 */ {1,22, 0x92,0xA5},
    /* 2024 */ {2,10, 0xB6,0xA5},
    /* 2025 */ {1,29, 0x55,0x25},
    /* 2026 */ {2,17, 0xAA,0x95},
    /* 2027 */ {2, 6, 0xB5,0x25},
    /* 2028 */ {1,26, 0x52,0xA5},
    /* 2029 */ {2,13, 0xE5,0x25},
    /* 2030 */ {2, 3, 0xD4,0xA5},
};
#define LUNAR_TABLE_BASE_YEAR 2020
#define LUNAR_TABLE_SIZE      (sizeof(s_lunar_table)/sizeof(s_lunar_table[0]))

static const char *s_lunar_month_names[] = {
    "正","二","三","四","五","六","七","八","九","十","冬","腊"
};
static const char *s_lunar_day_names[] = {
    "初一","初二","初三","初四","初五","初六","初七","初八","初九","初十",
    "十一","十二","十三","十四","十五","十六","十七","十八","十九","二十",
    "廿一","廿二","廿三","廿四","廿五","廿六","廿七","廿八","廿九","三十"
};
static const char *s_tg[] = {"甲","乙","丙","丁","戊","己","庚","辛","壬","癸"};
static const char *s_dz[] = {"子","丑","寅","卯","辰","巳","午","未","申","酉","戌","亥"};
static const char *s_sx[] = {"鼠","牛","虎","兔","龙","蛇","马","羊","猴","鸡","狗","猪"};

/* 24节气名称（按公历月份顺序，每月2个） */
static const char *s_solar_terms[] = {
    "小寒","大寒",   /* 1月 */
    "立春","雨水",   /* 2月 */
    "惊蛰","春分",   /* 3月 */
    "清明","谷雨",   /* 4月 */
    "立夏","小满",   /* 5月 */
    "芒种","夏至",   /* 6月 */
    "小暑","大暑",   /* 7月 */
    "立秋","处暑",   /* 8月 */
    "白露","秋分",   /* 9月 */
    "寒露","霜降",   /* 10月 */
    "立冬","小雪",   /* 11月 */
    "大雪","冬至",   /* 12月 */
};

/* 节气对应公历日期（近似，每月两个节气的大约日期） */
static const uint8_t s_term_days[][2] = {
    {6,20},{4,19},{6,21},{5,20},{6,21},{5,20},
    {5,20},{6,21},{6,21},{4,19},{6,21},{4,19},
};

/**
 * @brief 获取当月节气（返回空串表示无节气）
 */
static const char* get_solar_term(int year, int month, int day)
{
    (void)year;
    int idx = (month - 1) * 2;
    if (day == s_term_days[month-1][0]) return s_solar_terms[idx];
    if (day == s_term_days[month-1][1]) return s_solar_terms[idx+1];
    /* 前后一天容差 */
    if (day == s_term_days[month-1][0]+1 || day == s_term_days[month-1][0]-1)
        return s_solar_terms[idx];
    if (day == s_term_days[month-1][1]+1 || day == s_term_days[month-1][1]-1)
        return s_solar_terms[idx+1];
    return "";
}

/**
 * @brief 公历 -> 农历（简化算法，基于表格）
 */
static void solar_to_lunar(int sy, int sm, int sd,
                            int *ly, int *lm, int *ld,
                            int *leap_month, bool *is_leap)
{
    *ly = sy; *lm = 1; *ld = 1; *leap_month = 0; *is_leap = false;

    int tidx = sy - LUNAR_TABLE_BASE_YEAR;
    if (tidx < 0 || tidx >= (int)LUNAR_TABLE_SIZE) {
        /* 超出表格范围，直接返回默认 */
        *ly = sy - 1;
        return;
    }

    /* 正月初一的公历日期 */
    int base_month = s_lunar_table[tidx][0];
    int base_day   = s_lunar_table[tidx][1];
    uint16_t month_bits = ((uint16_t)s_lunar_table[tidx][2] << 8) | s_lunar_table[tidx][3];
    /* 高4bit = 闰月月份 */
    int lp = (month_bits >> 12) & 0x0F;
    /* 低12bit = 12个月大小 */
    uint16_t size_bits = month_bits & 0x0FFF;

    /* 计算公历到正月初一的天数差 */
    /* 简化：同年计算 */
    static const int days_in_month[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    int is_leap_year = (sy%4==0 && sy%100!=0) || sy%400==0;

    int day_of_sy = 0;
    for (int i = 1; i < sm; i++) {
        int dim = days_in_month[i];
        if (i == 2 && is_leap_year) dim = 29;
        day_of_sy += dim;
    }
    day_of_sy += sd;

    int day_of_base = 0;
    if (base_month == 1) {
        /* 正月初一在1月 */
        day_of_base = base_day;
    } else {
        /* 正月初一在2月 */
        int jan_days = days_in_month[1];
        day_of_base = jan_days + base_day;
    }

    int diff = day_of_sy - day_of_base; /* 距正月初一的天数，0=初一 */

    if (diff < 0) {
        /* 在上一年正月初一之前，用上一年表格 */
        tidx--;
        if (tidx < 0) { *ly = sy-1; return; }
        base_month = s_lunar_table[tidx][0];
        base_day   = s_lunar_table[tidx][1];
        month_bits = ((uint16_t)s_lunar_table[tidx][2] << 8) | s_lunar_table[tidx][3];
        lp = (month_bits >> 12) & 0x0F;
        size_bits = month_bits & 0x0FFF;

        int prev_is_leap = ((sy-1)%4==0 && (sy-1)%100!=0) || (sy-1)%400==0;
        int prev_day_of_base = 0;
        if (base_month == 1) {
            prev_day_of_base = base_day;
        } else {
            prev_day_of_base = days_in_month[1] + base_day;
        }
        /* 上一年到今年1月1日的天数 */
        int prev_year_days = prev_is_leap ? 366 : 365;
        diff = prev_year_days - prev_day_of_base + day_of_sy;
        *ly = sy - 1;
    } else {
        *ly = sy;
    }

    /* 逐月累加找农历月日 */
    int cur_month = 1;
    int remaining = diff;
    bool in_leap = false;
    for (int m = 0; m < 15; m++) {
        int is_big = (size_bits >> (11 - (m < 12 ? m : 11))) & 1;
        int days = is_big ? 30 : 29;
        if (m == 12 && lp > 0) {
            /* 有闰月，第12次循环是闰月 */
            is_big = (size_bits >> (11 - 11)) & 1;
            days = is_big ? 30 : 29;
        }
        if (remaining < days) {
            *lm = cur_month;
            *ld = remaining + 1;
            *leap_month = lp;
            *is_leap = in_leap;
            return;
        }
        remaining -= days;
        /* 判断是否到闰月 */
        if (lp > 0 && cur_month == lp && !in_leap) {
            in_leap = true;
        } else {
            in_leap = false;
            cur_month++;
        }
        if (cur_month > 12) break;
    }
    *lm = 12; *ld = 30;
}

/* ==================== SNTP 初始化 ==================== */
static bool s_sntp_initialized = false;

static void sntp_init_once(void)
{
    if (s_sntp_initialized) return;
    s_sntp_initialized = true;
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "ntp.aliyun.com");
    esp_sntp_setservername(1, "pool.ntp.org");
    esp_sntp_init();
    /* 设置北京时区 UTC+8 */
    setenv("TZ", "CST-8", 1);
    tzset();
    ESP_LOGI(TAG, "SNTP initialized");
}

/* ==================== 刷新日历内容 ==================== */
static void update_calendar_labels(void)
{
    time_t now;
    struct tm t;
    time(&now);
    localtime_r(&now, &t);

    int year  = t.tm_year + 1900;
    int month = t.tm_mon + 1;
    int day   = t.tm_mday;
    int hour  = t.tm_hour;
    int min   = t.tm_min;
    int wday  = t.tm_wday; /* 0=周日 */

    static const char *weekdays[] = {"周日","周一","周二","周三","周四","周五","周六"};

    /* 农历 */
    int ly, lm, ld, leap_m;
    bool is_leap;
    solar_to_lunar(year, month, day, &ly, &lm, &ld, &leap_m, &is_leap);

    /* 干支年 */
    int tg_idx = (ly - 4) % 10;
    int dz_idx = (ly - 4) % 12;
    if (tg_idx < 0) tg_idx += 10;
    if (dz_idx < 0) dz_idx += 12;

    char buf[64];

    /* 农历年份：甲子年(鼠) */
    if (s_label_lunar_year) {
        snprintf(buf, sizeof(buf), "%s%s年(%s)",
                 s_tg[tg_idx], s_dz[dz_idx], s_sx[dz_idx]);
        lv_label_set_text(s_label_lunar_year, buf);
    }

    /* 农历月日 */
    if (s_label_lunar_md) {
        snprintf(buf, sizeof(buf), "%s%s月  %s",
                 is_leap ? "闰" : "",
                 s_lunar_month_names[lm-1],
                 s_lunar_day_names[ld-1]);
        lv_label_set_text(s_label_lunar_md, buf);
    }

    /* 公历日期 + 星期 */
    if (s_label_solar) {
        snprintf(buf, sizeof(buf), "%d年%d月%d日  %s",
                 year, month, day, weekdays[wday]);
        lv_label_set_text(s_label_solar, buf);
    }

    /* 节气 */
    if (s_label_solar_term) {
        const char *term = get_solar_term(year, month, day);
        if (term && term[0] != '\0') {
            lv_label_set_text(s_label_solar_term, term);
            lv_obj_clear_flag(lv_obj_get_parent(s_label_solar_term), LV_OBJ_FLAG_HIDDEN);
        } else {
            /* 非节气日隐藏节气标签 */
            lv_obj_add_flag(lv_obj_get_parent(s_label_solar_term), LV_OBJ_FLAG_HIDDEN);
        }
    }

    /* 时间 */
    if (s_label_time) {
        snprintf(buf, sizeof(buf), "%02d:%02d", hour, min);
        lv_label_set_text(s_label_time, buf);
    }
}

/* ==================== LVGL 定时器回调 ==================== */
static void calendar_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    /* 检查时间是否已同步（时间戳 > 2020-01-01） */
    time_t now;
    time(&now);
    if (now < 1577836800) {
        /* 还未同步，显示"--:--" */
        if (s_label_time) lv_label_set_text(s_label_time, "--:--");
        return;
    }
    update_calendar_labels();
}

/* ==================== 导航栏回调 ==================== */
static void nav_btn_standby_cb(lv_event_t *e) { ui_switch_to_standby(); }
static void nav_btn_control_cb(lv_event_t *e) { ui_switch_to_control(); }
static void nav_btn_viewer_cb(lv_event_t *e)  { ui_switch_to_viewer();  }
static void simulate_wake_btn_cb(lv_event_t *e)
{
    ESP_LOGI(TAG, "唤醒词触发");
    ui_switch_to_avatar();
    voice_pipeline_trigger();
}
static void music_btn_cb(lv_event_t *e) { ui_switch_to_music(); }

/* ==================== 导航栏 ==================== */
static void create_nav_bar(lv_obj_t *parent)
{
    lv_obj_t *nav_bar = lv_obj_create(parent);
    lv_obj_set_size(nav_bar, LV_HOR_RES, 100);
    lv_obj_align(nav_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(nav_bar, THEME_CARD_BG, LV_PART_MAIN);
    lv_obj_set_style_border_width(nav_bar, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(nav_bar, THEME_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_border_opa(nav_bar, LV_OPA_30, LV_PART_MAIN);
    lv_obj_set_style_pad_all(nav_bar, 10, LV_PART_MAIN);
    lv_obj_set_flex_flow(nav_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(nav_bar, LV_FLEX_ALIGN_SPACE_AROUND,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    struct { const char *txt; void(*cb)(lv_event_t*); bool highlight; } btns[] = {
        {"息屏", nav_btn_standby_cb, false},
        {"主页", NULL,               true },
        {"便签", nav_btn_control_cb, false},
        {"查看", nav_btn_viewer_cb,  false},
    };

    for (int i = 0; i < 4; i++) {
        lv_obj_t *btn = lv_btn_create(nav_bar);
        lv_obj_set_size(btn, 120, 78);
        if (btns[i].highlight) ui_apply_button_light_style(btn);
        else                   ui_apply_button_dark_style(btn);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, btns[i].txt);
        lv_obj_set_style_text_font(lbl, THEME_FONT_CN, LV_PART_MAIN);
        lv_obj_center(lbl);

        if (btns[i].cb)
            lv_obj_add_event_cb(btn, btns[i].cb, LV_EVENT_CLICKED, NULL);
    }
}

/* ==================== 左侧农历面板 ==================== */
static lv_obj_t* create_lunar_panel(lv_obj_t *parent)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_set_width(panel, lv_pct(57));
    lv_obj_set_height(panel, lv_pct(100));
    lv_obj_set_style_radius(panel, THEME_RADIUS_LARGE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(panel, ALMANAC_PAPER_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 3, LV_PART_MAIN);
    lv_obj_set_style_border_color(panel, ALMANAC_PAPER_BORDER, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(panel, 18, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(panel, THEME_SHADOW_COLOR, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(panel, LV_OPA_20, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_x(panel, 4, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_y(panel, 4, LV_PART_MAIN);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(panel, 18, LV_PART_MAIN);
    lv_obj_set_style_pad_row(panel, 12, LV_PART_MAIN);

    /* 时间大字（最显眼）*/
    s_label_time = lv_label_create(panel);
    lv_label_set_text(s_label_time, "--:--");
    lv_obj_set_style_text_font(s_label_time, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_label_time, ALMANAC_RED, LV_PART_MAIN);

    /* 公历日期 + 星期 */
    s_label_solar = lv_label_create(panel);
    lv_label_set_text(s_label_solar, "正在同步时间...");
    lv_obj_set_style_text_font(s_label_solar, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_label_solar, ALMANAC_TEXT_GRAY, LV_PART_MAIN);
    lv_label_set_long_mode(s_label_solar, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_label_solar, lv_pct(95));

    /* 干支年份 */
    s_label_lunar_year = lv_label_create(panel);
    lv_label_set_text(s_label_lunar_year, "------年");
    lv_obj_set_style_text_font(s_label_lunar_year, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_label_lunar_year, ALMANAC_TEXT_GRAY, LV_PART_MAIN);

    /* 农历月日（大字） */
    s_label_lunar_md = lv_label_create(panel);
    lv_label_set_text(s_label_lunar_md, "农历 --月--");
    lv_obj_set_style_text_font(s_label_lunar_md, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_label_lunar_md, ALMANAC_TEXT_BLACK, LV_PART_MAIN);

    /* 节气标签（红色胶囊，非节气日隐藏） */
    lv_obj_t *term_container = lv_obj_create(panel);
    lv_obj_set_width(term_container, lv_pct(70));
    lv_obj_set_height(term_container, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(term_container, ALMANAC_RED, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(term_container, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(term_container, THEME_RADIUS_MEDIUM, LV_PART_MAIN);
    lv_obj_set_style_pad_all(term_container, 10, LV_PART_MAIN);
    lv_obj_set_style_border_width(term_container, 0, LV_PART_MAIN);

    s_label_solar_term = lv_label_create(term_container);
    lv_label_set_text(s_label_solar_term, "节气");
    lv_obj_set_style_text_font(s_label_solar_term, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_label_solar_term, THEME_TEXT_ON_DARK, LV_PART_MAIN);
    lv_obj_center(s_label_solar_term);

    /* 分隔线 */
    lv_obj_t *sep = lv_obj_create(panel);
    lv_obj_set_width(sep, lv_pct(90));
    lv_obj_set_height(sep, 2);
    lv_obj_set_style_bg_color(sep, ALMANAC_PAPER_BORDER, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(sep, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(sep, 0, LV_PART_MAIN);

    /* 宜/忌固定文字 */
    lv_obj_t *suitable = lv_label_create(panel);
    lv_label_set_text(suitable, "宜: 祈福  出行  嫁娶");
    lv_obj_set_style_text_font(suitable, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(suitable, ALMANAC_RED, LV_PART_MAIN);
    lv_label_set_long_mode(suitable, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(suitable, lv_pct(90));

    lv_obj_t *avoid = lv_label_create(panel);
    lv_label_set_text(avoid, "忌: 动土  安葬");
    lv_obj_set_style_text_font(avoid, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(avoid, ALMANAC_TEXT_GRAY, LV_PART_MAIN);
    lv_label_set_long_mode(avoid, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(avoid, lv_pct(90));

    return panel;
}

/* ==================== 右侧面板（天气 + 唤醒按钮） ==================== */
static lv_obj_t* create_right_panel(lv_obj_t *parent)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_set_width(panel, lv_pct(40));
    lv_obj_set_height(panel, lv_pct(100));
    lv_obj_set_style_radius(panel, THEME_RADIUS_LARGE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(panel, THEME_CARD_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(panel, LV_OPA_80, LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(panel, THEME_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_border_opa(panel, LV_OPA_30, LV_PART_MAIN);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(panel, 14, LV_PART_MAIN);
    lv_obj_set_style_pad_row(panel, 16, LV_PART_MAIN);

    /* 太阳圆圈 */
    lv_obj_t *sun = lv_obj_create(panel);
    lv_obj_set_size(sun, 120, 120);
    lv_obj_set_style_radius(sun, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(sun, ALMANAC_GOLD, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(sun, LV_OPA_80, LV_PART_MAIN);
    lv_obj_set_style_border_width(sun, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(sun, 20, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(sun, ALMANAC_GOLD, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(sun, LV_OPA_50, LV_PART_MAIN);

    lv_obj_t *sun_lbl = lv_label_create(sun);
    s_weather_icon_lbl = sun_lbl;
    lv_label_set_text(s_weather_icon_lbl, "...");
    lv_obj_set_style_text_font(s_weather_icon_lbl, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_weather_icon_lbl, THEME_TEXT_ON_DARK, LV_PART_MAIN);
    lv_obj_center(s_weather_icon_lbl);

    /* 温度 */
    s_weather_temp_lbl = lv_label_create(panel);
    lv_label_set_text(s_weather_temp_lbl, "--度");
    lv_obj_set_style_text_font(s_weather_temp_lbl, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_weather_temp_lbl, THEME_TEXT_PRIMARY, LV_PART_MAIN);

    /* 天气描述 */
    s_weather_desc_lbl = lv_label_create(panel);
    lv_label_set_text(s_weather_desc_lbl, "获取中...");
    lv_obj_set_style_text_font(s_weather_desc_lbl, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_weather_desc_lbl, THEME_TEXT_SECONDARY, LV_PART_MAIN);

    /* 湿度 */
    s_weather_hum_lbl = lv_label_create(panel);
    lv_label_set_text(s_weather_hum_lbl, "湿度 --%");
    lv_obj_set_style_text_font(s_weather_hum_lbl, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_weather_hum_lbl, THEME_TEXT_HINT, LV_PART_MAIN);

    /* 唤醒按钮 */
    lv_obj_t *wake_btn = lv_btn_create(panel);
    lv_obj_set_width(wake_btn, lv_pct(92));
    lv_obj_set_height(wake_btn, 72);
    ui_apply_button_light_style(wake_btn);
    lv_obj_t *wake_lbl = lv_label_create(wake_btn);
    lv_label_set_text(wake_lbl, "呼叫小柚子");
    lv_obj_set_style_text_font(wake_lbl, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(wake_lbl, THEME_TEXT_ON_DARK, LV_PART_MAIN);
    lv_obj_center(wake_lbl);
    lv_obj_add_event_cb(wake_btn, simulate_wake_btn_cb, LV_EVENT_CLICKED, NULL);

    /* 音乐按钮 */
    lv_obj_t *music_btn = lv_btn_create(panel);
    lv_obj_set_width(music_btn, lv_pct(92));
    lv_obj_set_height(music_btn, 72);
    lv_obj_set_style_radius(music_btn, THEME_RADIUS_MEDIUM, LV_PART_MAIN);
    lv_obj_set_style_bg_color(music_btn, lv_color_hex(0xCFAF47), LV_PART_MAIN);   /* 金色 */
    lv_obj_set_style_bg_opa(music_btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(music_btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(music_btn, 16, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(music_btn, lv_color_hex(0xCFAF47), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(music_btn, LV_OPA_20, LV_PART_MAIN);
    lv_obj_t *music_lbl = lv_label_create(music_btn);
    lv_label_set_text(music_lbl, "音乐");
    lv_obj_set_style_text_font(music_lbl, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(music_lbl, THEME_TEXT_ON_DARK, LV_PART_MAIN);
    lv_obj_center(music_lbl);
    lv_obj_add_event_cb(music_btn, music_btn_cb, LV_EVENT_CLICKED, NULL);

    return panel;
}

/* ==================== 创建主页 ==================== */
lv_obj_t* create_home_screen(void)
{
    ESP_LOGI(TAG, "Creating home screen with realtime clock...");

    /* 启动 SNTP */
    sntp_init_once();

    s_home_screen = lv_obj_create(NULL);
    lv_obj_set_size(s_home_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(s_home_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_home_screen, THEME_BG_COLOR, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_home_screen, LV_OPA_COVER, LV_PART_MAIN);

    /* 主容器 */
    s_almanac_container = lv_obj_create(s_home_screen);
    lv_obj_set_width(s_almanac_container, lv_pct(100));
    lv_obj_set_height(s_almanac_container, LV_VER_RES - 110);
    lv_obj_align(s_almanac_container, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_style_bg_opa(s_almanac_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_almanac_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_almanac_container, 8, LV_PART_MAIN);
    lv_obj_set_flex_flow(s_almanac_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(s_almanac_container, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(s_almanac_container, 8, LV_PART_MAIN);

    s_left_panel  = create_lunar_panel(s_almanac_container);
    s_right_panel = create_right_panel(s_almanac_container);

    create_nav_bar(s_home_screen);

    /* 立即刷新一次 */
    update_calendar_labels();

    /* 每 1 秒刷新一次时钟（SNTP同步后会准确） */
    if (s_time_timer) {
        lv_timer_del(s_time_timer);
    }
    s_time_timer = lv_timer_create(calendar_timer_cb, 1000, NULL);

    ESP_LOGI(TAG, "Home screen created OK");

    /* 启动天气后台任务（只启动一次）*/
    static bool s_weather_task_started = false;
    if (!s_weather_task_started) {
        s_weather_task_started = true;
        xTaskCreate(weather_fetch_task, "weather", 6144, NULL, 3, NULL);
    }

    return s_home_screen;
}

/* ==================== 删除主页 ==================== */
void delete_home_screen(void)
{
    if (s_time_timer) {
        lv_timer_del(s_time_timer);
        s_time_timer = NULL;
    }
    if (s_home_screen) {
        lv_obj_del(s_home_screen);
        s_home_screen       = NULL;
        s_almanac_container = NULL;
        s_left_panel        = NULL;
        s_right_panel       = NULL;
        s_label_lunar_year  = NULL;
        s_label_lunar_md    = NULL;
        s_label_solar       = NULL;
        s_label_solar_term  = NULL;
        s_label_time        = NULL;
    }
}
