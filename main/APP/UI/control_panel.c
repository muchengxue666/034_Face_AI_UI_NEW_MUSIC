/**
 ******************************************************************************
 * @file        control_panel.c
 * @version     V3.0
 * @brief       家庭留言便签板 - 实现文件
 ******************************************************************************
 * @attention   温馨的便签留言板界面
 *              - 浅暖黄色纸张背景
 *              - 可滚动的便签卡片列表
 *              - 新便签以动画形式插入
 *              - 大字体、宽行距（适老化设计）
 ******************************************************************************
 */

#include "control_panel.h"
#include "ai_assistant_ui.h"
#include "ui_theme.h"
#include "esp_lvgl_port.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "sticky_note_board";
/* 使用 LVGL 内置 CJK 字体替代自定义字体 */

/* ==================== 便签板颜色定义 - 继承主题暖色 ==================== */
#define NOTE_BOARD_BG           THEME_CARD_BG               /* 暖黄色便签板背景 */
#define NOTE_CARD_BG            THEME_CARD_LIGHT            /* 单张便签卡片背景 */
#define NOTE_CARD_SHADOW        THEME_SHADOW_COLOR          /* 便签阴影色 */
#define NOTE_TEXT_PRIMARY       THEME_TEXT_PRIMARY          /* 主文本深咖 */
#define NOTE_TEXT_SECONDARY     THEME_TEXT_SECONDARY        /* 副文本暖灰 */
#define NOTE_TEXT_TIME          THEME_TEXT_HINT             /* 时间文本浅暖灰 */
#define NOTE_ACCENT_COLOR       lv_color_hex(0xD97B75)      /* 强调色（柔和红）*/

/* ==================== 屏幕和控件对象 ==================== */
static lv_obj_t *s_control_screen = NULL;
static lv_obj_t *s_note_board = NULL;           /* 便签板主容器 */
static lv_obj_t *s_note_scroll = NULL;          /* 可滚动的便签列表 */
static lv_obj_t *s_title_label = NULL;          /* 标题 */

/* 便签计数 */
static int s_note_count = 0;

/* ==================== 模拟便签数据 ==================== */
typedef struct {
    const char *sender;
    const char *message;
    const char *time;
} sticky_note_data_t;

static const sticky_note_data_t s_sample_notes[] = {
    {"女儿", "爸,周末我带孩子来看你,记得准备好糖果!", "3小时前"},
    {"老伴", "记得按时吃药,晚饭在冰箱里热一下就好.", "昨天"},
    {"儿子", "爸,今天给您打了电话,您在睡觉,明天再聊!", "2天前"},
    {"孙女", "爷爷我想你啦!等放假就回去看你~", "上周"},
};
#define SAMPLE_NOTE_COUNT (sizeof(s_sample_notes) / sizeof(s_sample_notes[0]))

/* 模拟新消息 */
static const sticky_note_data_t s_new_notes[] = {
    {"女儿", "爸,今天天气好,记得出去走走!", "刚刚"},
    {"老伴", "隔壁王阿姨来家里坐坐,我去菜场了.", "刚刚"},
    {"儿子", "爸,公司忙完了,下周回家看您!", "刚刚"},
};
#define NEW_NOTE_COUNT (sizeof(s_new_notes) / sizeof(s_new_notes[0]))
static int s_new_note_idx = 0;

/* ==================== 导航栏回调函数 ==================== */
static void nav_btn_standby_cb(lv_event_t *e)
{
    ui_switch_to_standby();
}

static void nav_btn_home_cb(lv_event_t *e)
{
    ui_switch_to_home();
}

static void nav_btn_viewer_cb(lv_event_t *e)
{
    ui_switch_to_viewer();
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
    lv_obj_add_event_cb(btn_standby, nav_btn_standby_cb, LV_EVENT_CLICKED, NULL);

    /* 主页按钮 */
    lv_obj_t *btn_home = lv_btn_create(nav_bar);
    lv_obj_set_size(btn_home, 120, 78);
    ui_apply_button_dark_style(btn_home);
    lv_obj_t *label_home = lv_label_create(btn_home);
    lv_label_set_text(label_home, "主页");
    lv_obj_set_style_text_font(label_home, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_center(label_home);
    lv_obj_add_event_cb(btn_home, nav_btn_home_cb, LV_EVENT_CLICKED, NULL);

    /* 便签板按钮 - 高亮显示当前界面 */
    lv_obj_t *btn_control = lv_btn_create(nav_bar);
    lv_obj_set_size(btn_control, 120, 78);
    ui_apply_button_light_style(btn_control);
    lv_obj_t *label_control = lv_label_create(btn_control);
    lv_label_set_text(label_control, "便签");
    lv_obj_set_style_text_font(label_control, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_center(label_control);

    /* 查看器按钮 */
    lv_obj_t *btn_viewer = lv_btn_create(nav_bar);
    lv_obj_set_size(btn_viewer, 120, 78);
    ui_apply_button_dark_style(btn_viewer);
    lv_obj_t *label_viewer = lv_label_create(btn_viewer);
    lv_label_set_text(label_viewer, "查看");
    lv_obj_set_style_text_font(label_viewer, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_center(label_viewer);
    lv_obj_add_event_cb(btn_viewer, nav_btn_viewer_cb, LV_EVENT_CLICKED, NULL);
}

/**
 * @brief       新便签淡入动画完成回调
 * @param       a: 动画句柄
 * @retval      无
 */
static void note_anim_complete_cb(lv_anim_t *a)
{
    /* 动画完成，无需额外操作 */
}

/**
 * @brief       新便签透明度动画回调
 * @param       obj: 目标对象
 * @param       v: 当前动画值
 * @retval      无
 */
static void note_fade_in_cb(void *obj, int32_t v)
{
    lv_obj_set_style_opa((lv_obj_t *)obj, v, LV_PART_MAIN);
}

/**
 * @brief       创建单张便签卡片
 * @param       parent: 父对象
 * @param       sender: 发送者
 * @param       message: 消息内容
 * @param       time: 时间字符串
 * @param       animate: 是否使用动画
 * @retval      卡片对象指针
 */
static lv_obj_t* create_note_card(lv_obj_t *parent, const char *sender,
                                   const char *message, const char *time, bool animate)
{
    /* 便签卡片容器 */
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_width(card, lv_pct(95));
    lv_obj_set_height(card, LV_SIZE_CONTENT);

    /* 便签纸张效果 */
    lv_obj_set_style_bg_color(card, NOTE_CARD_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(card, THEME_RADIUS_MEDIUM, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(card, NOTE_CARD_SHADOW, LV_PART_MAIN);
    lv_obj_set_style_border_opa(card, LV_OPA_50, LV_PART_MAIN);

    /* 纸张阴影效果 */
    lv_obj_set_style_shadow_width(card, 12, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(card, NOTE_CARD_SHADOW, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(card, LV_OPA_40, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_x(card, 3, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_y(card, 3, LV_PART_MAIN);

    /* 卡片内边距 */
    lv_obj_set_style_pad_all(card, THEME_PAD_XLARGE, LV_PART_MAIN);
    lv_obj_set_style_pad_row(card, 12, LV_PART_MAIN);

    /* 使用Flex布局 */
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    /* ========== 消息内容（大字） ========== */
    lv_obj_t *msg_label = lv_label_create(card);
    lv_label_set_text(msg_label, message);
    lv_obj_set_width(msg_label, lv_pct(100));
    lv_obj_set_height(msg_label, LV_SIZE_CONTENT);
    lv_label_set_long_mode(msg_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(msg_label, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(msg_label, NOTE_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_line_space(msg_label, 8, LV_PART_MAIN);  /* 宽行距，适老化 */

    /* ========== 底部信息行：发送者 + 时间 ========== */
    lv_obj_t *info_row = lv_obj_create(card);
    lv_obj_set_width(info_row, lv_pct(100));
    lv_obj_set_height(info_row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(info_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(info_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(info_row, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(info_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(info_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* 发送者 */
    lv_obj_t *sender_label = lv_label_create(info_row);
    char sender_str[64];
    snprintf(sender_str, sizeof(sender_str), "-- %s", sender);
    lv_label_set_text(sender_label, sender_str);
    lv_obj_set_style_text_font(sender_label, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(sender_label, NOTE_ACCENT_COLOR, LV_PART_MAIN);

    /* 时间 */
    lv_obj_t *time_label = lv_label_create(info_row);
    lv_label_set_text(time_label, time);
    lv_obj_set_style_text_font(time_label, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(time_label, NOTE_TEXT_TIME, LV_PART_MAIN);

    /* ========== 如果需要动画 ========== */
    if (animate) {
        /* 初始透明 */
        lv_obj_set_style_opa(card, LV_OPA_TRANSP, LV_PART_MAIN);

        /* 淡入动画 */
        lv_anim_t fade_anim;
        lv_anim_init(&fade_anim);
        lv_anim_set_var(&fade_anim, card);
        lv_anim_set_exec_cb(&fade_anim, note_fade_in_cb);
        lv_anim_set_values(&fade_anim, LV_OPA_TRANSP, LV_OPA_COVER);
        lv_anim_set_time(&fade_anim, THEME_ANIM_NORMAL);
        lv_anim_set_ready_cb(&fade_anim, note_anim_complete_cb);
        lv_anim_start(&fade_anim);
    }

    s_note_count++;
    return card;
}

/**
 * @brief       模拟接收新便签按钮回调
 * @param       e: 事件句柄
 * @retval      无
 */
static void simulate_new_note_cb(lv_event_t *e)
{
    ESP_LOGI(TAG, "模拟接收新便签...");

    if (!s_note_scroll) return;

    /* 获取下一条模拟消息 */
    const sticky_note_data_t *note = &s_new_notes[s_new_note_idx % NEW_NOTE_COUNT];
    s_new_note_idx++;

    /* 创建新便签卡片（带动画） */
    lv_obj_t *new_card = create_note_card(s_note_scroll, note->sender, note->message, note->time, true);

    /* 将新卡片移动到列表顶部 */
    lv_obj_move_to_index(new_card, 0);

    /* 滚动到顶部显示新便签 */
    lv_obj_scroll_to_y(s_note_scroll, 0, LV_ANIM_ON);

    ESP_LOGI(TAG, "新便签已添加: [%s] %s", note->sender, note->message);
}

/**
 * @brief       创建家庭留言便签板
 * @param       无
 * @retval      屏幕对象指针
 */
lv_obj_t* create_control_panel(void)
{
    ESP_LOGI(TAG, "Creating sticky note board...");

    /* 创建屏幕 */
    s_control_screen = lv_obj_create(NULL);
    lv_obj_set_size(s_control_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(s_control_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_control_screen, THEME_BG_COLOR, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_control_screen, LV_OPA_COVER, LV_PART_MAIN);

    /* ========== 便签板主容器 ========== */
    s_note_board = lv_obj_create(s_control_screen);
    lv_obj_set_width(s_note_board, lv_pct(95));
    lv_obj_set_height(s_note_board, LV_VER_RES - 110);  /* 减去导航栏高度 */
    lv_obj_align(s_note_board, LV_ALIGN_TOP_MID, 0, 5);

    /* 暖黄色便签板背景 */
    lv_obj_set_style_bg_color(s_note_board, NOTE_BOARD_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_note_board, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(s_note_board, THEME_RADIUS_LARGE, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_note_board, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(s_note_board, NOTE_CARD_SHADOW, LV_PART_MAIN);

    /* 纸张阴影效果 */
    lv_obj_set_style_shadow_width(s_note_board, 20, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(s_note_board, THEME_SHADOW_COLOR, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(s_note_board, THEME_SHADOW_OPA, LV_PART_MAIN);

    /* Flex布局 */
    lv_obj_set_flex_flow(s_note_board, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_note_board, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(s_note_board, THEME_PAD_MEDIUM, LV_PART_MAIN);
    lv_obj_set_style_pad_row(s_note_board, THEME_PAD_MEDIUM, LV_PART_MAIN);
    lv_obj_clear_flag(s_note_board, LV_OBJ_FLAG_SCROLLABLE);

    /* ========== 标题栏 ========== */
    lv_obj_t *title_row = lv_obj_create(s_note_board);
    lv_obj_set_width(title_row, lv_pct(100));
    lv_obj_set_height(title_row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(title_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(title_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(title_row, THEME_PAD_SMALL, LV_PART_MAIN);
    lv_obj_set_flex_flow(title_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(title_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* 标题文本 */
    s_title_label = lv_label_create(title_row);
    lv_label_set_text(s_title_label, "来自家人的叮嘱");
    lv_obj_set_style_text_font(s_title_label, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_title_label, NOTE_TEXT_PRIMARY, LV_PART_MAIN);

    /* ========== 可滚动的便签列表 ========== */
    s_note_scroll = lv_obj_create(s_note_board);
    lv_obj_set_width(s_note_scroll, lv_pct(100));
    lv_obj_set_flex_grow(s_note_scroll, 1);  /* 占据剩余空间 */
    lv_obj_set_style_bg_opa(s_note_scroll, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_note_scroll, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_note_scroll, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(s_note_scroll, THEME_PAD_MEDIUM, LV_PART_MAIN);

    /* Flex布局，垂直排列 */
    lv_obj_set_flex_flow(s_note_scroll, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_note_scroll, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* 添加示例便签 */
    for (int i = 0; i < SAMPLE_NOTE_COUNT; i++) {
        create_note_card(s_note_scroll,
                        s_sample_notes[i].sender,
                        s_sample_notes[i].message,
                        s_sample_notes[i].time,
                        false);  /* 初始便签不需要动画 */
    }

    /* ========== 模拟接收新便签按钮 ========== */
    lv_obj_t *new_note_btn = lv_btn_create(s_note_board);
    lv_obj_set_width(new_note_btn, lv_pct(80));
    lv_obj_set_height(new_note_btn, 50);
    lv_obj_set_style_bg_color(new_note_btn, NOTE_ACCENT_COLOR, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(new_note_btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(new_note_btn, THEME_RADIUS_MEDIUM, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(new_note_btn, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(new_note_btn, NOTE_ACCENT_COLOR, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(new_note_btn, LV_OPA_30, LV_PART_MAIN);

    lv_obj_t *btn_label = lv_label_create(new_note_btn);
    lv_label_set_text(btn_label, "模拟接收新便签");
    lv_obj_set_style_text_font(btn_label, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(btn_label, THEME_TEXT_ON_DARK, LV_PART_MAIN);
    lv_obj_center(btn_label);

    lv_obj_add_event_cb(new_note_btn, simulate_new_note_cb, LV_EVENT_CLICKED, NULL);

    /* 添加底部导航栏 */
    create_nav_bar(s_control_screen);

    ESP_LOGI(TAG, "Sticky note board created successfully with %d notes", s_note_count);
    return s_control_screen;
}

/**
 * @brief       删除家庭留言便签板
 * @param       无
 * @retval      无
 */
void delete_control_panel(void)
{
    if (s_control_screen) {
        lv_obj_del(s_control_screen);
        s_control_screen = NULL;
        s_note_board = NULL;
        s_note_scroll = NULL;
        s_title_label = NULL;
        s_note_count = 0;
    }
}

/**
 * @brief       模拟接收新便签（外部接口）
 * @param       sender: 发送者名称
 * @param       message: 消息内容
 * @param       time: 时间字符串
 * @retval      无
 */
void sticky_note_add_message(const char *sender, const char *message, const char *time)
{
    if (!s_note_scroll) {
        ESP_LOGW(TAG, "Note scroll not initialized");
        return;
    }

    ESP_LOGI(TAG, "Adding new note from external call: [%s] %s", sender, message);

    /* 创建新便签卡片（带动画） */
    lv_obj_t *new_card = create_note_card(s_note_scroll, sender, message, time, true);

    /* 将新卡片移动到列表顶部 */
    lv_obj_move_to_index(new_card, 0);

    /* 滚动到顶部显示新便签 */
    lv_obj_scroll_to_y(s_note_scroll, 0, LV_ANIM_ON);
}
