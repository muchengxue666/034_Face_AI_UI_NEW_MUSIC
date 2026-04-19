/**
 ******************************************************************************
 * @file        viewer_screen.c
 * @version     V2.0
 * @brief       知识库影像视图 - 完整实现
 ******************************************************************************
 * @attention   支持图片展示、手势交互、滑动与放大动画
 ******************************************************************************
 */

#include "viewer_screen.h"
#include "ai_assistant_ui.h"
#include "ui_theme.h"
#include "esp_lvgl_port.h"
#include "esp_log.h"
#include "dht22.h"
#include "smoke_sensor.h"
#include <stdlib.h>
#include <stdio.h>

static const char *TAG = "viewer_screen";

/* 使用 LVGL 内置 CJK 字体替代自定义字体 */

/* 屏幕对象 */
static lv_obj_t *s_viewer_screen = NULL;

/* 传感器显示标签 */
static lv_obj_t *s_temp_label   = NULL;   /* 温度 */
static lv_obj_t *s_humi_label   = NULL;   /* 湿度 */
static lv_obj_t *s_smoke_label  = NULL;   /* 烟雾浓度 */
static lv_obj_t *s_smoke_bar    = NULL;   /* 烟雾进度条 */
static lv_timer_t *s_sensor_timer = NULL; /* 传感器刷新定时器 */

/* 图片相关 */
static lv_obj_t *s_image_container = NULL;
static lv_obj_t *s_image_placeholder = NULL;
static uint8_t s_current_image_idx = 0;
static bool s_is_animating = false;

/* 动画延迟更新的上下文 */
typedef struct {
    lv_anim_t slide_in_anim;
} slide_animation_ctx_t;

#define MAX_IMAGES 4

/* 前置声明 */
static void update_image_display(uint8_t index);
static void viewer_zoom_complete_cb(lv_anim_t *a);

/* 全局动画上下文 */
static slide_animation_ctx_t *s_slide_anim_ctx = NULL;

/**
 * @brief       图片切换动画回调 - 滑出
 * @param       a: 动画句柄
 * @param       v: 当前X位置
 * @retval      无
 */
static void image_slide_out_cb(lv_anim_t *a, int32_t v)
{
    if (s_image_placeholder) {
        lv_obj_set_x(s_image_placeholder, v);
    }
}

/**
 * @brief       图片滑入动画回调
 * @param       a: 动画句柄
 * @param       v: 当前X位置
 * @retval      无
 */
static void image_slide_in_cb(lv_anim_t *a, int32_t v)
{
    if (s_image_placeholder) {
        lv_obj_set_x(s_image_placeholder, v);
    }
}

/**
 * @brief       图片放大/还原动画回调
 * @param       a: 动画句柄
 * @param       v: 当前缩放值 (256=1.0x)
 * @retval      无
 */
static void image_zoom_cb(lv_anim_t *a, int32_t v)
{
    if (s_image_placeholder) {
        lv_img_set_zoom(s_image_placeholder, v);
    }
}

/**
 * @brief       动画延迟更新计时器回调
 * @param       timer: 计时器句柄
 * @retval      无
 */
static void image_update_timer_cb(lv_timer_t *timer)
{
    update_image_display(s_current_image_idx);
    lv_obj_set_x(s_image_placeholder, LV_HOR_RES);
    lv_timer_del(timer);
}

/**
 * @brief       动画延迟启动计时器回调
 * @param       timer: 计时器句柄
 * @retval      无
 */
static void image_slide_start_timer_cb(lv_timer_t *timer)
{
    if (s_slide_anim_ctx) {
        lv_anim_start(&s_slide_anim_ctx->slide_in_anim);
    }
    lv_timer_del(timer);
}

/**
 * @brief       更新图片占位内容
 * @param       index: 图片索引
 * @retval      无
 */
static void update_image_display(uint8_t index)
{
    if (!s_image_placeholder) return;
    
    /* 此处可以加载实际的图片资源，目前使用占位符 */
    char buf[128];
    snprintf(buf, sizeof(buf), "文档 %d\n\n"
                               "图片区域\n\n"
                               "当前: %d / %d",
             index + 1, index + 1, MAX_IMAGES);
    
    lv_label_set_text(s_image_placeholder, buf);
    
    ESP_LOGI(TAG, "Displaying image %d", index + 1);
}

/**
 * @brief       切换到下一张图片（模拟挥手手势）
 * @param       无
 * @retval      无
 */
static void switch_to_next_image(void)
{
    if (s_is_animating) return;
    
    s_is_animating = true;
    s_current_image_idx = (s_current_image_idx + 1) % MAX_IMAGES;
    
    ESP_LOGI(TAG, "Switching to next image: %d", s_current_image_idx);
    
    /* 当前图片向左滑出 */
    lv_anim_t slide_out_anim;
    lv_anim_init(&slide_out_anim);
    lv_anim_set_var(&slide_out_anim, s_image_placeholder);
    lv_anim_set_exec_cb(&slide_out_anim, (lv_anim_exec_xcb_t)image_slide_out_cb);
    lv_anim_set_values(&slide_out_anim, 0, -LV_HOR_RES);
    lv_anim_set_time(&slide_out_anim, THEME_ANIM_SLOW);
    lv_anim_set_path_cb(&slide_out_anim, lv_anim_path_ease_in);
    lv_anim_start(&slide_out_anim);
    
    /* 延迟后更新图片 */
    lv_timer_create(image_update_timer_cb, THEME_ANIM_SLOW / 2, NULL);
    
    /* 新图片从右滑入 */
    s_slide_anim_ctx = (slide_animation_ctx_t *)malloc(sizeof(slide_animation_ctx_t));
    if (s_slide_anim_ctx) {
        lv_anim_init(&s_slide_anim_ctx->slide_in_anim);
        lv_anim_set_var(&s_slide_anim_ctx->slide_in_anim, s_image_placeholder);
        lv_anim_set_exec_cb(&s_slide_anim_ctx->slide_in_anim, (lv_anim_exec_xcb_t)image_slide_in_cb);
        lv_anim_set_values(&s_slide_anim_ctx->slide_in_anim, LV_HOR_RES, 0);
        lv_anim_set_time(&s_slide_anim_ctx->slide_in_anim, THEME_ANIM_SLOW);
        lv_anim_set_path_cb(&s_slide_anim_ctx->slide_in_anim, lv_anim_path_ease_out);
        lv_anim_set_ready_cb(&s_slide_anim_ctx->slide_in_anim, viewer_zoom_complete_cb);
        
        /* 延迟启动滑入动画 */
        lv_timer_create(image_slide_start_timer_cb, THEME_ANIM_SLOW / 2, NULL);
    }
}

/**
 * @brief       图片缩放/滑动动画完成回调
 * @param       a: 动画句柄
 * @retval      无
 */
static void viewer_zoom_complete_cb(lv_anim_t *a)
{
    s_is_animating = false;
    /* 清理动画上下文 */
    if (s_slide_anim_ctx) {
        free(s_slide_anim_ctx);
        s_slide_anim_ctx = NULL;
    }
}

/**
 * @brief       放大/还原图片（模拟握拳手势）
 * @param       无
 * @retval      无
 */
static void toggle_image_zoom(void)
{
    if (s_is_animating || !s_image_placeholder) return;
    
    s_is_animating = true;
    
    /* 获取当前缩放值 */
    uint16_t current_zoom = lv_img_get_zoom(s_image_placeholder);
    uint16_t target_zoom = (current_zoom == 256) ? 400 : 256;  /* 256=1.0x, 512=2.0x */
    
    ESP_LOGI(TAG, "Toggling zoom: %d -> %d", current_zoom, target_zoom);
    
    /* 创建缩放动画 */
    lv_anim_t zoom_anim;
    lv_anim_init(&zoom_anim);
    lv_anim_set_var(&zoom_anim, s_image_placeholder);
    lv_anim_set_exec_cb(&zoom_anim, (lv_anim_exec_xcb_t)image_zoom_cb);
    lv_anim_set_values(&zoom_anim, current_zoom, target_zoom);
    lv_anim_set_time(&zoom_anim, THEME_ANIM_SLOW);
    lv_anim_set_path_cb(&zoom_anim, lv_anim_path_ease_in_out);
    lv_anim_set_ready_cb(&zoom_anim, viewer_zoom_complete_cb);
    lv_anim_start(&zoom_anim);
}

/**
 * @brief       模拟挥手手势按钮回调
 * @param       e: 事件句柄
 * @retval      无
 */
static void wave_gesture_btn_cb(lv_event_t *e)
{
    ESP_LOGI(TAG, "Wave gesture detected - switching image");
    switch_to_next_image();
}

/**
 * @brief       模拟握拳手势按钮回调
 * @param       e: 事件句柄
 * @retval      无
 */
static void fist_gesture_btn_cb(lv_event_t *e)
{
    ESP_LOGI(TAG, "Fist gesture detected - zooming image");
    toggle_image_zoom();
}

/**
 * @brief       返回按钮回调
 * @param       e: 事件句柄
 * @retval      无
 */
static void viewer_back_btn_cb(lv_event_t *e)
{
    ui_switch_to_home();
}

/**
 * @brief       创建图片显示区域
 * @param       parent: 父对象
 * @retval      图片容器对象指针
 */
static lv_obj_t* create_image_display(lv_obj_t *parent)
{
    /* 图片展示容器 */
    s_image_container = lv_obj_create(parent);
    lv_obj_set_width(s_image_container, LV_HOR_RES - THEME_PAD_LARGE * 2);
    lv_obj_set_height(s_image_container, LV_VER_RES - 300);
    lv_obj_align(s_image_container, LV_ALIGN_TOP_MID, 0, THEME_PAD_LARGE);
    ui_apply_card_style(s_image_container, THEME_OPA_FULL);
    
    /* 图片占位符（标签模拟） */
    s_image_placeholder = lv_label_create(s_image_container);
    lv_label_set_text(s_image_placeholder, "文档 1\n\n"
                                           "图片区域\n\n"
                                           "当前: 1 / 4\n\n"
                                           "[等待图片加载]");
    lv_obj_center(s_image_placeholder);
    lv_obj_set_style_text_color(s_image_placeholder, THEME_TEXT_SECONDARY, LV_PART_MAIN);
    lv_obj_set_style_text_align(s_image_placeholder, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(s_image_placeholder, THEME_FONT_CN, LV_PART_MAIN);
    
    /* 使用img对象代替（实际图片应在这里加载） */
    // lv_obj_t *actual_image = lv_img_create(s_image_container);
    // lv_img_set_src(actual_image, "path/to/xray.bin");
    // lv_obj_center(actual_image);
    
    return s_image_container;
}

/* ========== 导航栏回调和创建函数 ========== */
static void viewer_nav_control_cb(lv_event_t *e)
{
    ui_switch_to_control();
}

static void viewer_nav_standby_cb(lv_event_t *e)
{
    ui_switch_to_standby();
}

/**
 * @brief       创建底部导航栏
 * @param       parent: 父对象
 * @retval      无
 */
static void create_nav_bar(lv_obj_t *parent)
{
    /* 导航栏容器 */
    lv_obj_t *nav_bar = lv_obj_create(parent);
    lv_obj_set_size(nav_bar, LV_HOR_RES, 100);
    lv_obj_align(nav_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(nav_bar, THEME_CARD_BG, LV_PART_MAIN);
    lv_obj_set_style_border_width(nav_bar, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(nav_bar, THEME_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_border_opa(nav_bar, LV_OPA_30, LV_PART_MAIN);
    lv_obj_set_flex_flow(nav_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(nav_bar, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(nav_bar, 8, LV_PART_MAIN);
    
    /* 息屏按钮 */
    lv_obj_t *btn_standby = lv_btn_create(nav_bar);
    lv_obj_set_size(btn_standby, 120, 78);
    ui_apply_button_dark_style(btn_standby);
    lv_obj_t *label_standby = lv_label_create(btn_standby);
    lv_label_set_text(label_standby, "息屏");
    lv_obj_set_style_text_font(label_standby, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_center(label_standby);
    lv_obj_add_event_cb(btn_standby, viewer_nav_standby_cb, LV_EVENT_CLICKED, NULL);

    /* 主页按钮 */
    lv_obj_t *btn_home = lv_btn_create(nav_bar);
    lv_obj_set_size(btn_home, 120, 78);
    ui_apply_button_dark_style(btn_home);
    lv_obj_t *label_home = lv_label_create(btn_home);
    lv_label_set_text(label_home, "主页");
    lv_obj_set_style_text_font(label_home, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_center(label_home);
    lv_obj_add_event_cb(btn_home, viewer_back_btn_cb, LV_EVENT_CLICKED, NULL);

    /* 控制面板按钮 */
    lv_obj_t *btn_control = lv_btn_create(nav_bar);
    lv_obj_set_size(btn_control, 120, 78);
    ui_apply_button_dark_style(btn_control);
    lv_obj_t *label_control = lv_label_create(btn_control);
    lv_label_set_text(label_control, "便签");
    lv_obj_set_style_text_font(label_control, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_center(label_control);
    lv_obj_add_event_cb(btn_control, viewer_nav_control_cb, LV_EVENT_CLICKED, NULL);

    /* 查看器按钮 - 高亮显示当前界面 */
    lv_obj_t *btn_viewer = lv_btn_create(nav_bar);
    lv_obj_set_size(btn_viewer, 120, 78);
    ui_apply_button_light_style(btn_viewer);
    lv_obj_t *label_viewer = lv_label_create(btn_viewer);
    lv_label_set_text(label_viewer, "查看");
    lv_obj_set_style_text_font(label_viewer, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_center(label_viewer);
}

/**
 * @brief       创建手势识别按钮组
 * @param       parent: 父对象
 * @retval      无
 */
static void create_gesture_buttons(lv_obj_t *parent)
{
    /* 按钮容器 */
    lv_obj_t *btn_container = lv_obj_create(parent);
    lv_obj_set_width(btn_container, LV_HOR_RES - THEME_PAD_LARGE * 2);
    lv_obj_set_height(btn_container, 130);
    lv_obj_align(btn_container, LV_ALIGN_BOTTOM_MID, 0, -THEME_PAD_LARGE);
    lv_obj_set_style_bg_opa(btn_container, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_container, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, 
                          LV_FLEX_ALIGN_CENTER);
    
    /* 挥手（下一页）按钮 */
    lv_obj_t *wave_btn = lv_btn_create(btn_container);
    lv_obj_set_width(wave_btn, (LV_HOR_RES / 2) - THEME_PAD_MEDIUM);
    lv_obj_set_height(wave_btn, 72);
    ui_apply_button_light_style(wave_btn);
    lv_obj_add_event_cb(wave_btn, wave_gesture_btn_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *wave_label = lv_label_create(wave_btn);
    lv_label_set_text(wave_label, "挥手\n(下一张)");
    lv_obj_center(wave_label);
    lv_obj_set_style_text_align(wave_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(wave_label, THEME_TEXT_ON_DARK, LV_PART_MAIN);
    lv_obj_set_style_text_font(wave_label, THEME_FONT_CN, LV_PART_MAIN);

    /* 握拳（放大）按钮 */
    lv_obj_t *fist_btn = lv_btn_create(btn_container);
    lv_obj_set_width(fist_btn, (LV_HOR_RES / 2) - THEME_PAD_MEDIUM);
    lv_obj_set_height(fist_btn, 72);
    ui_apply_button_light_style(fist_btn);
    lv_obj_add_event_cb(fist_btn, fist_gesture_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *fist_label = lv_label_create(fist_btn);
    lv_label_set_text(fist_label, "握拳\n(缩放)");
    lv_obj_center(fist_label);
    lv_obj_set_style_text_align(fist_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(fist_label, THEME_TEXT_ON_DARK, LV_PART_MAIN);
    lv_obj_set_style_text_font(fist_label, THEME_FONT_CN, LV_PART_MAIN);
}

/**
 * @brief       传感器定时刷新回调（每 3 秒）
 */
static void sensor_refresh_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    /* 读取 DHT22 */
    dht22_data_t dht;
    if (dht22_read(&dht) == ESP_OK) {
        char temp_buf[24];
        char humi_buf[24];
        snprintf(temp_buf, sizeof(temp_buf), "%.1f 度", dht.temperature);
        snprintf(humi_buf, sizeof(humi_buf), "%.1f %%", dht.humidity);
        if (s_temp_label) lv_label_set_text(s_temp_label, temp_buf);
        if (s_humi_label) lv_label_set_text(s_humi_label, humi_buf);
    }

    /* 读取烟雾传感器 */
    int smoke_raw = smoke_sensor_read_raw();
    if (smoke_raw >= 0) {
        int mv = smoke_sensor_read_mv();
        bool alarm = smoke_sensor_is_alarm();
        if (s_smoke_label) {
            if (alarm) {
                lv_label_set_text_fmt(s_smoke_label, "%d (%dmV) !!!", smoke_raw, mv);
                lv_obj_set_style_text_color(s_smoke_label, lv_color_hex(0xFF0000), LV_PART_MAIN);
            } else {
                lv_label_set_text_fmt(s_smoke_label, "%d (%dmV)", smoke_raw, mv);
                lv_obj_set_style_text_color(s_smoke_label, THEME_TEXT_PRIMARY, LV_PART_MAIN);
            }
        }
        if (s_smoke_bar) {
            lv_bar_set_value(s_smoke_bar, smoke_raw > 4095 ? 100 : smoke_raw * 100 / 4095, LV_ANIM_ON);
            if (alarm) {
                lv_obj_set_style_bg_color(s_smoke_bar, lv_color_hex(0xFF0000), LV_PART_INDICATOR);
            } else {
                lv_obj_set_style_bg_color(s_smoke_bar, THEME_ACCENT_GOLD, LV_PART_INDICATOR);
            }
        }
    }

}

/**
 * @brief       创建传感器显示面板
 */
static void create_sensor_panel(lv_obj_t *parent)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_set_width(panel, LV_HOR_RES - THEME_PAD_LARGE * 2);
    lv_obj_set_height(panel, 280);
    lv_obj_align(panel, LV_ALIGN_TOP_MID, 0, 60);
    ui_apply_card_style(panel, THEME_OPA_FULL);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(panel, 20, LV_PART_MAIN);
    lv_obj_set_style_pad_row(panel, 12, LV_PART_MAIN);

    /* 标题 */
    lv_obj_t *title = lv_label_create(panel);
    lv_label_set_text(title, "环境传感器");
    lv_obj_set_style_text_font(title, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, THEME_TEXT_PRIMARY, LV_PART_MAIN);

    /* --- 温度行 --- */
    lv_obj_t *temp_row = lv_obj_create(panel);
    lv_obj_set_width(temp_row, lv_pct(100));
    lv_obj_set_height(temp_row, 40);
    lv_obj_set_style_bg_opa(temp_row, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(temp_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(temp_row, 0, LV_PART_MAIN);

    lv_obj_t *temp_icon = lv_label_create(temp_row);
    lv_label_set_text(temp_icon, "温度:");
    lv_obj_set_style_text_font(temp_icon, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(temp_icon, THEME_TEXT_SECONDARY, LV_PART_MAIN);
    lv_obj_align(temp_icon, LV_ALIGN_LEFT_MID, 0, 0);

    s_temp_label = lv_label_create(temp_row);
    lv_label_set_text(s_temp_label, "-- 度");
    lv_obj_set_style_text_font(s_temp_label, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_temp_label, THEME_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_align(s_temp_label, LV_ALIGN_RIGHT_MID, 0, 0);

    /* --- 湿度行 --- */
    lv_obj_t *humi_row = lv_obj_create(panel);
    lv_obj_set_width(humi_row, lv_pct(100));
    lv_obj_set_height(humi_row, 40);
    lv_obj_set_style_bg_opa(humi_row, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(humi_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(humi_row, 0, LV_PART_MAIN);

    lv_obj_t *humi_icon = lv_label_create(humi_row);
    lv_label_set_text(humi_icon, "湿度:");
    lv_obj_set_style_text_font(humi_icon, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(humi_icon, THEME_TEXT_SECONDARY, LV_PART_MAIN);
    lv_obj_align(humi_icon, LV_ALIGN_LEFT_MID, 0, 0);

    s_humi_label = lv_label_create(humi_row);
    lv_label_set_text(s_humi_label, "-- %");
    lv_obj_set_style_text_font(s_humi_label, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_humi_label, THEME_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_align(s_humi_label, LV_ALIGN_RIGHT_MID, 0, 0);

    /* --- 烟雾行 --- */
    lv_obj_t *smoke_row = lv_obj_create(panel);
    lv_obj_set_width(smoke_row, lv_pct(100));
    lv_obj_set_height(smoke_row, 40);
    lv_obj_set_style_bg_opa(smoke_row, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(smoke_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(smoke_row, 0, LV_PART_MAIN);

    lv_obj_t *smoke_icon = lv_label_create(smoke_row);
    lv_label_set_text(smoke_icon, "烟雾:");
    lv_obj_set_style_text_font(smoke_icon, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(smoke_icon, THEME_TEXT_SECONDARY, LV_PART_MAIN);
    lv_obj_align(smoke_icon, LV_ALIGN_LEFT_MID, 0, 0);

    s_smoke_label = lv_label_create(smoke_row);
    lv_label_set_text(s_smoke_label, "-- (--mV)");
    lv_obj_set_style_text_font(s_smoke_label, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_smoke_label, THEME_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_align(s_smoke_label, LV_ALIGN_RIGHT_MID, 0, 0);

    /* --- 烟雾进度条 --- */
    s_smoke_bar = lv_bar_create(panel);
    lv_obj_set_width(s_smoke_bar, lv_pct(100));
    lv_obj_set_height(s_smoke_bar, 16);
    lv_bar_set_range(s_smoke_bar, 0, 100);
    lv_bar_set_value(s_smoke_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_smoke_bar, lv_color_hex(0xE8D5B5), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_smoke_bar, THEME_ACCENT_GOLD, LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_smoke_bar, 8, LV_PART_MAIN);
    lv_obj_set_style_radius(s_smoke_bar, 8, LV_PART_INDICATOR);

    /* 启动定时刷新（每 3 秒）*/
    s_sensor_timer = lv_timer_create(sensor_refresh_timer_cb, 3000, NULL);
}

/**
 * @brief       创建知识库影像视图页面
 * @param       无
 * @retval      屏幕对象指针
 */
lv_obj_t* create_viewer_screen(void)
{
    ESP_LOGI(TAG, "Creating viewer screen...");
    
    /* 创建屏幕 */
    s_viewer_screen = lv_obj_create(NULL);
    lv_obj_set_size(s_viewer_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(s_viewer_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_viewer_screen, THEME_BG_COLOR, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_viewer_screen, LV_OPA_COVER, LV_PART_MAIN);
    
    /* 顶部标题 */
    lv_obj_t *title = lv_label_create(s_viewer_screen);
    lv_label_set_text(title, "知识库");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, THEME_PAD_LARGE);
    ui_apply_title_style(title);
    lv_obj_set_style_text_font(title, THEME_FONT_CN, LV_PART_MAIN);
    
    /* 创建传感器显示面板 */
    create_sensor_panel(s_viewer_screen);

    /* 创建图片显示区域（传感器面板下方）*/
    create_image_display(s_viewer_screen);
    /* 调整图片区域位置到传感器面板下面 */
    lv_obj_align(s_image_container, LV_ALIGN_TOP_MID, 0, 360);
    
    /* 创建手势按钮 */
    create_gesture_buttons(s_viewer_screen);
    
    /* 添加底部导航栏 */
    create_nav_bar(s_viewer_screen);
    
    ESP_LOGI(TAG, "Viewer screen created successfully");
    return s_viewer_screen;
}

/**
 * @brief       删除知识库影像视图页面
 * @param       无
 * @retval      无
 */
void delete_viewer_screen(void)
{
    if (s_sensor_timer) {
        lv_timer_del(s_sensor_timer);
        s_sensor_timer = NULL;
    }
    if (s_viewer_screen) {
        lv_obj_del(s_viewer_screen);
        s_viewer_screen = NULL;
        s_image_container = NULL;
        s_image_placeholder = NULL;
        s_temp_label = NULL;
        s_humi_label = NULL;
        s_smoke_label = NULL;
        s_smoke_bar = NULL;
    }
}

