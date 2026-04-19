/**
 ******************************************************************************
 * @file        standby_screen.c
 * @version     V2.0
 * @brief       息屏唤醒页 - 与格完整实现
 ******************************************************************************
 * @attention   暖深棕背景下的多层呼吸光晕效果 + 点击唤醒转场
 ******************************************************************************
 */

#include "standby_screen.h"
#include "ai_assistant_ui.h"
#include "ui_theme.h"
#include "esp_lvgl_port.h"
#include "esp_log.h"

static const char *TAG = "standby_screen";
/* 使用 LVGL 内置 CJK 字体替代自定义字体 */

static lv_obj_t *s_standby_screen = NULL;
static lv_obj_t *s_halo_outer = NULL;        /* 外层光晕 */
static lv_obj_t *s_halo_middle = NULL;       /* 中层光晕 */
static lv_obj_t *s_halo_inner = NULL;        /* 内层光晕 */
static lv_obj_t *s_icon_center = NULL;       /* 中心图标 */
static bool s_is_waking_up = false;

/* 光晕尺寸 */
#define HALO_OUTER_SIZE         240
#define HALO_MIDDLE_SIZE        180
#define HALO_INNER_SIZE         120
#define ICON_SIZE               60

/**
 * @brief       外层光晕呼吸动画回调
 * @param       obj: 目标对象
 * @param       v: 当前动画值 (0-255)
 * @retval      无
 */
static void breathing_outer_cb(void *obj, int32_t v)
{
    if (s_halo_outer) {
        lv_obj_set_style_bg_opa(s_halo_outer, v, LV_PART_MAIN);
    }
}

/**
 * @brief       中层光晕呼吸动画回调
 * @param       obj: 目标对象
 * @param       v: 当前动画值 (0-255)
 * @retval      无
 */
static void breathing_middle_cb(void *obj, int32_t v)
{
    if (s_halo_middle) {
        lv_obj_set_style_bg_opa(s_halo_middle, v, LV_PART_MAIN);
    }
}

/**
 * @brief       内层光晕呼吸动画回调
 * @param       obj: 目标对象
 * @param       v: 当前动画值 (0-255)
 * @retval      无
 */
static void breathing_inner_cb(void *obj, int32_t v)
{
    if (s_halo_inner) {
        lv_obj_set_style_bg_opa(s_halo_inner, v, LV_PART_MAIN);
    }
}

/**
 * @brief       中心图标脉冲动画回调
 * @param       obj: 目标对象
 * @param       v: 当前动画值
 * @retval      无
 */
static void icon_pulse_cb(void *obj, int32_t v)
{
    if (s_icon_center) {
        lv_obj_set_style_bg_opa(s_icon_center, v, LV_PART_MAIN);
    }
}

/**
 * @brief       屏幕点击事件回调
 * @param       e: 事件句柄
 * @retval      无
 */
static void standby_click_cb(lv_event_t *e)
{
    if (s_is_waking_up) return;

    s_is_waking_up = true;
    ESP_LOGI(TAG, "Wake-up triggered!");

    /* 停止呼吸动画 */
    lv_anim_del(s_halo_outer, breathing_outer_cb);
    lv_anim_del(s_halo_middle, breathing_middle_cb);
    lv_anim_del(s_halo_inner, breathing_inner_cb);
    lv_anim_del(s_icon_center, icon_pulse_cb);

    /* 直接切换到化身界面（滑屏动画由 lv_scr_load_anim 处理）*/
    ui_switch_to_home();
    s_is_waking_up = false;
}

/**
 * @brief       启动呼吸动画
 * @param       无
 * @retval      无
 */
static void start_breathing_animations(void)
{
    /* 外层光晕呼吸（最慢，透明度最低） */
    lv_anim_t outer_anim;
    lv_anim_init(&outer_anim);
    lv_anim_set_var(&outer_anim, s_halo_outer);
    lv_anim_set_exec_cb(&outer_anim, breathing_outer_cb);
    lv_anim_set_values(&outer_anim, 10, 40);
    lv_anim_set_time(&outer_anim, 3000);
    lv_anim_set_playback_time(&outer_anim, 3000);
    lv_anim_set_repeat_count(&outer_anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&outer_anim);
    
    /* 中层光晕呼吸 */
    lv_anim_t middle_anim;
    lv_anim_init(&middle_anim);
    lv_anim_set_var(&middle_anim, s_halo_middle);
    lv_anim_set_exec_cb(&middle_anim, breathing_middle_cb);
    lv_anim_set_values(&middle_anim, 20, 60);
    lv_anim_set_time(&middle_anim, 2500);
    lv_anim_set_playback_time(&middle_anim, 2500);
    lv_anim_set_repeat_count(&middle_anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&middle_anim);
    
    /* 内层光晕呼吸（最快，透明度最高） */
    lv_anim_t inner_anim;
    lv_anim_init(&inner_anim);
    lv_anim_set_var(&inner_anim, s_halo_inner);
    lv_anim_set_exec_cb(&inner_anim, breathing_inner_cb);
    lv_anim_set_values(&inner_anim, 40, 100);
    lv_anim_set_time(&inner_anim, 2000);
    lv_anim_set_playback_time(&inner_anim, 2000);
    lv_anim_set_repeat_count(&inner_anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&inner_anim);
    
    /* 中心图标脉冲 */
    lv_anim_t icon_anim;
    lv_anim_init(&icon_anim);
    lv_anim_set_var(&icon_anim, s_icon_center);
    lv_anim_set_exec_cb(&icon_anim, icon_pulse_cb);
    lv_anim_set_values(&icon_anim, 80, 200);
    lv_anim_set_time(&icon_anim, 1500);
    lv_anim_set_playback_time(&icon_anim, 1500);
    lv_anim_set_repeat_count(&icon_anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&icon_anim);
}

/**
 * @brief       创建息屏唤醒页
 * @param       无
 * @retval      屏幕对象指针
 */
lv_obj_t* create_standby_screen(void)
{
    ESP_LOGI(TAG, "Creating standby screen...");
    
    /* 创建屏幕对象 */
    s_standby_screen = lv_obj_create(NULL);
    lv_obj_set_size(s_standby_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(s_standby_screen, LV_OBJ_FLAG_SCROLLABLE);
    
    /* 移除所有继承的样式，确保底色暖深棕 */
    lv_obj_remove_style_all(s_standby_screen);

    /* 设置暖深棕背景（替代纯黑）*/
    lv_obj_set_style_bg_color(s_standby_screen, THEME_BG_DARK, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_standby_screen, LV_OPA_COVER, LV_PART_MAIN);
    
    /* 添加点击事件 */
    lv_obj_add_event_cb(s_standby_screen, standby_click_cb, LV_EVENT_CLICKED, NULL);
    
    /* ========== 创建多层呼吸光晕 ========== */
    
    /* 外层光晕 - 大圆，最淡 */
    s_halo_outer = lv_obj_create(s_standby_screen);
    lv_obj_set_size(s_halo_outer, HALO_OUTER_SIZE, HALO_OUTER_SIZE);
    lv_obj_align(s_halo_outer, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(s_halo_outer, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_halo_outer, THEME_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_halo_outer, 10, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_halo_outer, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(s_halo_outer, 0, LV_PART_MAIN);
    
    /* 中层光晕 - 中圆，中等透明度 */
    s_halo_middle = lv_obj_create(s_standby_screen);
    lv_obj_set_size(s_halo_middle, HALO_MIDDLE_SIZE, HALO_MIDDLE_SIZE);
    lv_obj_align(s_halo_middle, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(s_halo_middle, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_halo_middle, THEME_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_halo_middle, 20, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_halo_middle, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(s_halo_middle, 12, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(s_halo_middle, THEME_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(s_halo_middle, LV_OPA_20, LV_PART_MAIN);
    
    /* 内层光晕 - 小圆，最亮 */
    s_halo_inner = lv_obj_create(s_standby_screen);
    lv_obj_set_size(s_halo_inner, HALO_INNER_SIZE, HALO_INNER_SIZE);
    lv_obj_align(s_halo_inner, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(s_halo_inner, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_halo_inner, THEME_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_halo_inner, 40, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_halo_inner, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(s_halo_inner, THEME_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_border_opa(s_halo_inner, LV_OPA_30, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(s_halo_inner, 16, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(s_halo_inner, THEME_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(s_halo_inner, LV_OPA_40, LV_PART_MAIN);
    
    /* 中心图标 - 圆点或符号 */
    s_icon_center = lv_obj_create(s_standby_screen);
    lv_obj_set_size(s_icon_center, ICON_SIZE, ICON_SIZE);
    lv_obj_align(s_icon_center, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(s_icon_center, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_icon_center, THEME_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_icon_center, 80, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_icon_center, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(s_icon_center, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(s_icon_center, THEME_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(s_icon_center, LV_OPA_60, LV_PART_MAIN);
    
    /* 启动呼吸动画 */
    start_breathing_animations();
    
    /* 提示文本（可选） */
    lv_obj_t *hint_label = lv_label_create(s_standby_screen);
    lv_label_set_text(hint_label, "点击屏幕唤醒");
    lv_obj_align(hint_label, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_obj_set_style_text_color(hint_label, lv_color_hex(0xB8A898), LV_PART_MAIN);  /* 暖灰（深背景提示） */
    lv_obj_set_style_text_font(hint_label, THEME_FONT_CN, LV_PART_MAIN);
    
    ESP_LOGI(TAG, "Standby screen created successfully");
    return s_standby_screen;
}

/**
 * @brief       删除息屏唤醒页
 * @param       无
 * @retval      无
 */
void delete_standby_screen(void)
{
    if (s_standby_screen) {
        lv_obj_del(s_standby_screen);
        s_standby_screen = NULL;
        s_halo_outer = NULL;
        s_halo_middle = NULL;
        s_halo_inner = NULL;
        s_icon_center = NULL;
    }
}

/**
 * @brief       重新启动呼吸动画
 * @param       无
 * @retval      无
 */
void restart_breathing_animations(void)
{
    /* 停止现有动画 */
    lv_anim_del(s_halo_outer, breathing_outer_cb);
    lv_anim_del(s_halo_middle, breathing_middle_cb);
    lv_anim_del(s_halo_inner, breathing_inner_cb);
    lv_anim_del(s_icon_center, icon_pulse_cb);
    
    /* 重新启动呼吸动画 */
    start_breathing_animations();
    
    ESP_LOGI(TAG, "Breathing animations restarted");
}

/**
 * @param       无
 * @retval      无
 */
void trigger_standby_wake_up(void)
{
    if (s_standby_screen && !s_is_waking_up) {
        /* 创建事件并调用点击回调 */
        lv_event_t e = {
            .target = s_standby_screen,
            .current_target = s_standby_screen,
            .code = LV_EVENT_CLICKED
        };
        standby_click_cb(&e);
    }
}
