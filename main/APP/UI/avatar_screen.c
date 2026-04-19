/**
 ******************************************************************************
 * @file        avatar_screen.c
 * @version     V1.0
 * @brief       小柚子化身界面 - 实现文件
 ******************************************************************************
 * @attention   唤醒后显示的可爱拟人化形象"小柚子"
 *              - 黑色背景上的圆形占位图
 *              - 上下浮动动画
 *              - 点击或 3 秒后自动跳转到 home_screen
 *              - 预留 lv_rlottie 接口用于未来 Lottie 动画
 ******************************************************************************
 */

#include "avatar_screen.h"
#include "ai_assistant_ui.h"
#include "ui_theme.h"
#include "esp_lvgl_port.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "lvgl.h"
#include <stdio.h>

static const char *TAG = "avatar_screen";
/* 使用 LVGL 内置 CJK 字体替代自定义字体 */

/* ==================== 屏幕和控件对象 ==================== */
static lv_obj_t *s_avatar_screen = NULL;
static lv_obj_t *s_avatar_container = NULL;
static lv_obj_t *s_avatar_circle = NULL;
static lv_obj_t *s_avatar_gif = NULL;          /* GIF 动画对象 */
static lv_obj_t *s_greeting_label = NULL;
static lv_obj_t *s_hint_label = NULL;

/* ==================== 定时器和动画 ==================== */
static lv_timer_t *s_auto_transition_timer = NULL;  /* 自动跳转定时器 */
static bool s_avatar_animating = false;
static bool s_voice_active = false;                 /* 语音流水线是否正在运行 */

/* ==================== 化身尺寸定义 ==================== */
#define AVATAR_CIRCLE_SIZE      220
#define AVATAR_INNER_SIZE       200
#define FLOAT_AMPLITUDE         15      /* 浮动振幅（像素） */
#define FLOAT_DURATION          1500    /* 浮动周期（毫秒） */
#define AUTO_TRANSITION_MS      5000    /* 自动跳转时间（毫秒） */

/* 小柚子配色 - 使用主题定义 */
/* AVATAR_GOLD 已移至 ui_theme.h 中定义为 THEME_ACCENT_GOLD */

/* ==================== 问候语数组 ==================== */
static const char *s_greetings[] = {
    "你好呀!我是小柚子~",
    "嘿嘿,又见面啦!",
    "今天心情怎么样？",
    "我在这里陪着你!",
    "有什么想聊的吗？"
};
#define GREETING_COUNT (sizeof(s_greetings) / sizeof(s_greetings[0]))

/**
 * @brief       浮动动画回调 - Y轴位移
 * @param       obj: 目标对象
 * @param       v: 当前动画值
 * @retval      无
 */
static void float_anim_cb(void *obj, int32_t v)
{
    if (s_avatar_gif) {
        lv_obj_align(s_avatar_gif, LV_ALIGN_CENTER, 0, -30 + v - FLOAT_AMPLITUDE);
    }
}

/**
 * @brief       呼吸光晕动画回调 - 透明度
 * @param       obj: 目标对象
 * @param       v: 当前动画值 (0-255)
 * @retval      无
 */
static void glow_anim_cb(void *obj, int32_t v)
{
    if (s_avatar_gif) {
        lv_obj_set_style_opa(s_avatar_gif, v, LV_PART_MAIN);
    }
}

/**
 * @brief       启动化身动画
 * @param       无
 * @retval      无
 */
static void start_avatar_animations(void)
{
    if (s_avatar_animating) return;
    s_avatar_animating = true;

    /* ========== 上下浮动动画 ========== */
    lv_anim_t float_anim;
    lv_anim_init(&float_anim);
    lv_anim_set_var(&float_anim, s_avatar_container);
    lv_anim_set_exec_cb(&float_anim, float_anim_cb);
    lv_anim_set_values(&float_anim, 0, FLOAT_AMPLITUDE * 2);
    lv_anim_set_time(&float_anim, FLOAT_DURATION);
    lv_anim_set_playback_time(&float_anim, FLOAT_DURATION);
    lv_anim_set_repeat_count(&float_anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&float_anim, lv_anim_path_ease_in_out);
    lv_anim_start(&float_anim);

    /* ========== 呼吸动画（GIF透明度 200~255，轻微闪烁）========== */
    lv_anim_t glow_anim;
    lv_anim_init(&glow_anim);
    lv_anim_set_var(&glow_anim, s_avatar_gif);
    lv_anim_set_exec_cb(&glow_anim, glow_anim_cb);
    lv_anim_set_values(&glow_anim, 200, 255);
    lv_anim_set_time(&glow_anim, 1200);
    lv_anim_set_playback_time(&glow_anim, 1200);
    lv_anim_set_repeat_count(&glow_anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&glow_anim);
}

/**
 * @brief       自动跳转定时器回调
 * @param       timer: 定时器句柄
 * @retval      无
 */
static void auto_transition_timer_cb(lv_timer_t *timer)
{
    if (s_voice_active) {
        /* 语音流水线还在运行，不跳转，重新排定定时器 */
        ESP_LOGI(TAG, "Voice active, postponing auto transition");
        s_auto_transition_timer = lv_timer_create(auto_transition_timer_cb, AUTO_TRANSITION_MS, NULL);
        lv_timer_set_repeat_count(s_auto_transition_timer, 1);
        return;
    }
    ESP_LOGI(TAG, "Auto transition to home screen");
    ui_switch_to_home();
}

/**
 * @brief       屏幕点击事件回调
 * @param       e: 事件句柄
 * @retval      无
 */
static void avatar_click_cb(lv_event_t *e)
{
    ESP_LOGI(TAG, "Avatar clicked, switching to home screen");

    /* 点击时立即跳转，取消自动跳转定时器 */
    if (s_auto_transition_timer) {
        lv_timer_del(s_auto_transition_timer);
        s_auto_transition_timer = NULL;
    }

    ui_switch_to_home();
}

/**
 * @brief       创建小柚子化身界面
 * @param       无
 * @retval      屏幕对象指针
 */
lv_obj_t* create_avatar_screen(void)
{
    ESP_LOGI(TAG, "Creating avatar screen...");

    /* 创建屏幕对象 */
    s_avatar_screen = lv_obj_create(NULL);
    lv_obj_set_size(s_avatar_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(s_avatar_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_style_all(s_avatar_screen);
    lv_obj_set_style_bg_color(s_avatar_screen, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_avatar_screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_add_event_cb(s_avatar_screen, avatar_click_cb, LV_EVENT_CLICKED, NULL);

    /* ========== GIF直接挂在screen顶层，居中偏上 ========== */
    /* 先验证文件能否打开 */
    FILE *f = fopen("/spiffs/xiaoyouzi.gif", "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fclose(f);
        ESP_LOGI(TAG, "GIF file OK, size=%ld bytes", sz);
    } else {
        ESP_LOGE(TAG, "GIF file NOT FOUND at /spiffs/xiaoyouzi.gif !");
    }

    ESP_LOGI(TAG, "Free heap: %d, Free PSRAM: %d",
             heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

    s_avatar_gif = lv_gif_create(s_avatar_screen);
    lv_gif_set_src(s_avatar_gif, "S:/spiffs/xiaoyouzi.gif");
    /* 不强制set_size，让GIF按原始帧尺寸自适应；GIF文件本身应为220×220 */
    lv_obj_align(s_avatar_gif, LV_ALIGN_CENTER, 0, -30);
    lv_obj_clear_flag(s_avatar_gif, LV_OBJ_FLAG_SCROLLABLE);
    ESP_LOGI(TAG, "GIF object created, w=%d h=%d",
             lv_obj_get_width(s_avatar_gif), lv_obj_get_height(s_avatar_gif));

    /* s_avatar_circle 和 s_avatar_container 复用为同一对象，仅用于浮动动画 */
    s_avatar_circle    = s_avatar_gif;
    s_avatar_container = s_avatar_gif;

    /* ========== 问候语，GIF下方20px ========== */
    s_greeting_label = lv_label_create(s_avatar_screen);
    uint32_t idx = lv_rand(0, GREETING_COUNT - 1);
    lv_label_set_text(s_greeting_label, s_greetings[idx]);
    lv_obj_set_style_text_font(s_greeting_label, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_greeting_label, THEME_TEXT_ON_DARK, LV_PART_MAIN);
    lv_obj_set_style_text_align(s_greeting_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align_to(s_greeting_label, s_avatar_gif, LV_ALIGN_OUT_BOTTOM_MID, 0, 35);

    /* ========== 底部提示 ========== */
    s_hint_label = lv_label_create(s_avatar_screen);
    lv_label_set_text(s_hint_label, "点击屏幕继续");
    lv_obj_align(s_hint_label, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_obj_set_style_text_font(s_hint_label, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_hint_label, lv_color_hex(0xB8A898), LV_PART_MAIN);

    /* ========== 启动动画 ========== */
    start_avatar_animations();

    /* ========== 设置自动跳转定时器 ========== */
    s_auto_transition_timer = lv_timer_create(auto_transition_timer_cb, AUTO_TRANSITION_MS, NULL);
    lv_timer_set_repeat_count(s_auto_transition_timer, 1);

    ESP_LOGI(TAG, "Avatar screen created successfully");
    return s_avatar_screen;
}

/**
 * @brief       删除小柚子化身界面
 * @param       无
 * @retval      无
 */
void delete_avatar_screen(void)
{
    /* 先停止动画和定时器 */
    stop_avatar_animations();

    if (s_avatar_screen) {
        lv_obj_del(s_avatar_screen);
        s_avatar_screen = NULL;
        s_avatar_container = NULL;
        s_avatar_circle = NULL;
        s_avatar_gif = NULL;
        s_greeting_label = NULL;
        s_hint_label = NULL;
    }
}

/**
 * @brief       停止化身动画
 * @param       无
 * @retval      无
 */
void stop_avatar_animations(void)
{
    /* 停止浮动动画 */
    lv_anim_del(s_avatar_gif, float_anim_cb);
    lv_anim_del(s_avatar_gif, glow_anim_cb);

    /* 停止自动跳转定时器 */
    if (s_auto_transition_timer) {
        lv_timer_del(s_auto_transition_timer);
        s_auto_transition_timer = NULL;
    }

    s_avatar_animating = false;
}

/**
 * @brief       重新启动化身动画
 * @param       无
 * @retval      无
 */
void restart_avatar_animations(void)
{
    stop_avatar_animations();
    start_avatar_animations();

    /* 重新设置自动跳转定时器 */
    s_auto_transition_timer = lv_timer_create(auto_transition_timer_cb, AUTO_TRANSITION_MS, NULL);
    lv_timer_set_repeat_count(s_auto_transition_timer, 1);

    ESP_LOGI(TAG, "Avatar animations restarted");
}

/**
 * @brief       通知 avatar 界面语音流水线开始/结束
 * @param       active: true=语音开始(阻止自动跳转), false=语音结束(允许跳转)
 */
void avatar_set_voice_active(bool active)
{
    s_voice_active = active;
    ESP_LOGI(TAG, "Voice active = %s", active ? "true" : "false");
}
