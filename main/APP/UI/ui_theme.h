/**
 ******************************************************************************
 * @file        ui_theme.h
 * @version     V2.0
 * @brief       统一主题与样式定义 - 极致柔和与温润（Soft & Warm）
 ******************************************************************************
 * @attention   适老化设计：莫兰迪暖纸色系，大圆角，弥散阴影
 *              禁用纯白(#FFFFFF)和纯黑(#000000)
 ******************************************************************************
 */

#ifndef __UI_THEME_H__
#define __UI_THEME_H__

#include "lvgl.h"

/* ==================== 颜色定义 - 暖纸色系 ==================== */
/* 背景色 - 宣纸暖白 */
#define THEME_BG_COLOR          lv_color_hex(0xFDF6E3)  /* 宣纸暖白 */
#define THEME_BG_DARK           lv_color_hex(0x2D2520)  /* 夜间暖深棕（替代纯黑）*/
#define THEME_BG_ALT            lv_color_hex(0xF5ECD8)  /* 稍暗暖白 */
#define THEME_CARD_BG           lv_color_hex(0xFFF9E6)  /* 暖黄纸 */
#define THEME_CARD_LIGHT        lv_color_hex(0xFFFDF5)  /* 浅卡片背景 */

/* 主题强调色 - 暖橙桃木 */
#define THEME_PRIMARY           lv_color_hex(0xDDA77B)  /* 暖橙（主强调）*/
#define THEME_PRIMARY_DARK      lv_color_hex(0xC4915E)  /* 深暖橙 */
#define THEME_SECONDARY         lv_color_hex(0xE8B2A1)  /* 浅桃木（次强调）*/
#define THEME_ACCENT_RED        lv_color_hex(0xC41E3A)  /* 中国红（老黄历用）*/
#define THEME_ACCENT_GOLD       lv_color_hex(0xCFAF47)  /* 金色（小柚子用）*/

/* 文字色 - 深咖暖灰 */
#define THEME_TEXT_PRIMARY      lv_color_hex(0x5C4B43)  /* 深咖色 - 主文本 */
#define THEME_TEXT_SECONDARY    lv_color_hex(0x7A6B5E)  /* 暖灰棕 - 副文本 */
#define THEME_TEXT_HINT         lv_color_hex(0xA89888)  /* 浅暖灰 - 提示文本 */
#define THEME_TEXT_ON_DARK      lv_color_hex(0xFFF8E7)  /* 暖白（深色背景用）*/

/* 阴影色 */
#define THEME_SHADOW_COLOR      lv_color_hex(0xC4B5A0)  /* 暖米棕阴影 */

/* 功能色（保留，微调饱和度）*/
#define THEME_SUCCESS           lv_color_hex(0x7CB88A)  /* 柔和绿 */
#define THEME_WARNING           lv_color_hex(0xE5A858)  /* 柔和橙 */
#define THEME_ERROR             lv_color_hex(0xD97B75)  /* 柔和红 */
#define THEME_INFO              lv_color_hex(0x7BA8C4)  /* 柔和蓝 */

/* AI 对话相关 */
#define THEME_AI_MSG_BG         lv_color_hex(0xDDA77B)  /* AI 消息背景 - 暖橙 */
#define THEME_USER_MSG_BG       lv_color_hex(0x7CB88A)  /* 用户消息背景 - 柔和绿 */

/* ==================== 尺寸定义 - 大圆角 ==================== */
#define THEME_RADIUS_SMALL      12      /* 原 4 → 12 */
#define THEME_RADIUS_MEDIUM     24      /* 原 12 → 24 */
#define THEME_RADIUS_LARGE      32      /* 原 20 → 32 */
#define THEME_RADIUS_CIRCLE     LV_RADIUS_CIRCLE

/* 阴影 - 更大更柔 */
#define THEME_SHADOW_WIDTH      20      /* 原 8 → 20 */
#define THEME_SHADOW_OPA        LV_OPA_10   /* 原 30 → 10（极淡）*/

/* 内边距 */
#define THEME_PAD_SMALL         10
#define THEME_PAD_MEDIUM        18
#define THEME_PAD_LARGE         28
#define THEME_PAD_XLARGE        36

/* 边框 - 禁用生硬边框 */
#define THEME_BORDER_WIDTH      0

/* ==================== 自定义字体（支持完整中文字符集）==================== */
LV_FONT_DECLARE(myFont24);
#define THEME_FONT_CN           (&myFont24)   /* 全局中文字体，24px */

/* ==================== 字体大小 ==================== */
#define THEME_FONT_TINY         10
#define THEME_FONT_SMALL        14
#define THEME_FONT_MEDIUM       18
#define THEME_FONT_LARGE        22
#define THEME_FONT_XLARGE       28
#define THEME_FONT_TITLE        36

/* ==================== 动画时长（毫秒）==================== */
#define THEME_ANIM_FAST         100
#define THEME_ANIM_NORMAL       300
#define THEME_ANIM_SLOW         500
#define THEME_ANIM_VERY_SLOW    800

/* 屏幕过渡动画时长（毫秒）- 适老化柔和节奏 */
#define THEME_SCREEN_ANIM_TIME  350

/* ==================== 透明度 ==================== */
#define THEME_OPA_FULL          LV_OPA_COVER
#define THEME_OPA_SEMITRANS     LV_OPA_80
#define THEME_OPA_HALF          LV_OPA_50
#define THEME_OPA_LIGHT         LV_OPA_30
#define THEME_OPA_VERY_LIGHT    LV_OPA_20

/* ==================== 高级样式辅助函数 ==================== */

/**
 * @brief       应用卡片样式（暖纸风格）
 * @param       obj: 目标对象
 * @param       opa: 透明度（0-255）
 * @retval      无
 */
static inline void ui_apply_card_style(lv_obj_t *obj, lv_opa_t opa)
{
    lv_obj_set_style_radius(obj, THEME_RADIUS_MEDIUM, LV_PART_MAIN);
    lv_obj_set_style_bg_color(obj, THEME_CARD_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, opa, LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, THEME_BORDER_WIDTH, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(obj, THEME_SHADOW_WIDTH, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(obj, THEME_SHADOW_COLOR, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(obj, THEME_SHADOW_OPA, LV_PART_MAIN);
    lv_obj_set_style_pad_all(obj, THEME_PAD_MEDIUM, LV_PART_MAIN);
}

/**
 * @brief       应用按钮样式（暖橙色）
 * @param       obj: 目标对象
 * @retval      无
 */
static inline void ui_apply_button_light_style(lv_obj_t *obj)
{
    lv_obj_set_style_radius(obj, THEME_RADIUS_MEDIUM, LV_PART_MAIN);
    lv_obj_set_style_bg_color(obj, THEME_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, THEME_OPA_FULL, LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, THEME_BORDER_WIDTH, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(obj, THEME_SHADOW_WIDTH, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(obj, THEME_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(obj, THEME_SHADOW_OPA, LV_PART_MAIN);
    lv_obj_set_style_text_color(obj, THEME_TEXT_ON_DARK, LV_PART_MAIN);
}

/**
 * @brief       应用按钮样式（浅色/卡片色）
 * @param       obj: 目标对象
 * @retval      无
 */
static inline void ui_apply_button_dark_style(lv_obj_t *obj)
{
    lv_obj_set_style_radius(obj, THEME_RADIUS_MEDIUM, LV_PART_MAIN);
    lv_obj_set_style_bg_color(obj, THEME_CARD_LIGHT, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, THEME_OPA_FULL, LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, THEME_BORDER_WIDTH, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(obj, THEME_SHADOW_WIDTH, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(obj, THEME_SHADOW_COLOR, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(obj, THEME_SHADOW_OPA, LV_PART_MAIN);
    lv_obj_set_style_text_color(obj, THEME_TEXT_PRIMARY, LV_PART_MAIN);
}

/**
 * @brief       应用标题文本样式
 * @param       obj: 目标对象
 * @retval      无
 */
static inline void ui_apply_title_style(lv_obj_t *obj)
{
    lv_obj_set_style_text_color(obj, THEME_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(obj, THEME_FONT_CN, LV_PART_MAIN);
}

/**
 * @brief       应用正文文本样式
 * @param       obj: 目标对象
 * @retval      无
 */
static inline void ui_apply_body_style(lv_obj_t *obj)
{
    lv_obj_set_style_text_color(obj, THEME_TEXT_SECONDARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(obj, THEME_FONT_CN, LV_PART_MAIN);
}

/**
 * @brief       应用深色背景样式（用于待机页、化身页）
 * @param       obj: 目标对象
 * @retval      无
 */
static inline void ui_apply_dark_bg_style(lv_obj_t *obj)
{
    lv_obj_set_style_bg_color(obj, THEME_BG_DARK, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, THEME_OPA_FULL, LV_PART_MAIN);
}

#endif /* __UI_THEME_H__ */
