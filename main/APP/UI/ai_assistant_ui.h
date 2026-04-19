/**
 ******************************************************************************
 * @file        ai_assistant_ui.h
 * @version     V1.0
 * @brief       AI助手现代化UI框架 - 主头文件
 ******************************************************************************
 * @attention   基于LVGL v8.3+的现代化卡片式UI框架
 *              支持多模态交互、硬件控制、RAG知识库检索
 ******************************************************************************
 */

#ifndef __AI_ASSISTANT_UI_H
#define __AI_ASSISTANT_UI_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "lvgl.h"
#include "esp_heap_caps.h"
#include "gesture_ai.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 调试模式开关 - 设置为1可强制进入调试模式 */
#define FORCE_DEBUG_MODE 0

/* UI状态枚举 */
typedef enum {
    UI_STATE_STANDBY,           // 息屏唤醒状态
    UI_STATE_AVATAR,            // 小柚子化身界面（唤醒后过渡）
    UI_STATE_HOME,              // 多模态交互主页（老黄历）
    UI_STATE_CONTROL,           // 家庭留言便签板
    UI_STATE_VIEWER,            // 知识库与影像视图
    UI_STATE_MUSIC              // 音乐播放界面
} ui_state_t;

/* 手势类型定义在 gesture_ai.h 中，此处不再重复定义 */

/* 硬件控制消息 */
typedef struct {
    uint8_t brightness;         // 屏幕亮度 (0-100)
    bool fan_state;             // 排风扇状态
} hardware_control_msg_t;

/* 流式文本消息 */
typedef struct {
    char character;             // 单个字符
    bool is_complete;           // 是否为完整消息结束
} stream_text_msg_t;

/* 手势控制消息 */
typedef struct {
    gesture_type_t gesture;     // 手势类型
    int16_t x, y;               // 手势位置坐标
} gesture_msg_t;

/* UI框架全局句柄 */
typedef struct {
    lv_obj_t *current_screen;   // 当前显示的屏幕
    ui_state_t current_state;   // 当前UI状态
    
    // 消息队列句柄
    QueueHandle_t text_queue;   // 流式文本队列
    QueueHandle_t hardware_queue; // 硬件控制队列
    QueueHandle_t gesture_queue;  // 手势事件队列
    
    // 同步信号量
    SemaphoreHandle_t ui_mutex; // UI互斥锁
    
    // 各界面句柄缓存
    lv_obj_t *standby_screen;
    lv_obj_t *avatar_screen;    // 小柚子化身界面
    lv_obj_t *home_screen;
    lv_obj_t *control_panel;
    lv_obj_t *viewer_screen;
    lv_obj_t *music_screen;     // 音乐播放界面

    // 全局手势反馈指示器
    lv_obj_t *gesture_feedback;
    lv_timer_t *gesture_hide_timer;
    
    // 动画句柄
    lv_anim_t wake_up_anim;
    lv_anim_t breathing_anim;
    
    // 视觉区域控件
    lv_obj_t *camera_view;
    lv_obj_t *gesture_indicator;
    
    // 对话区域控件
    lv_obj_t *chat_container;
    lv_obj_t *current_bubble;
    lv_obj_t *audio_waveform;
    
    // 硬件控制控件
    lv_obj_t *brightness_slider;
    lv_obj_t *fan_switch;
    
    // 影像查看控件
    lv_obj_t *image_viewer;
    uint16_t current_zoom;      // 当前缩放比例
    
} ai_ui_handle_t;

/* 全局UI句柄 */
extern ai_ui_handle_t g_ai_ui;

/* 函数声明 */
void ai_assistant_ui_init(void);
void ai_assistant_ui_deinit(void);

/* 状态切换函数 */
void ui_switch_to_standby(void);
void ui_switch_to_avatar(void);     /* 新增：切换到小柚子化身界面 */
void ui_switch_to_home(void);
void ui_switch_to_control(void);
void ui_switch_to_viewer(void);
void ui_switch_to_music(void);      /* 新增：切换到音乐播放界面 */

/* 外部触发接口 */
void ui_trigger_wake_up(void);
void ui_update_streaming_text(char ch, bool is_complete);
void ui_update_hardware_status(uint8_t brightness, bool fan_on);
void ui_handle_gesture_event(gesture_type_t gesture, int16_t x, int16_t y);

/* 全局手势反馈 */
void ui_show_gesture_feedback(const char *text);    /* 新增：显示手势反馈提示 */
void ui_hide_gesture_feedback(void);                /* 新增：隐藏手势反馈提示 */

/* 样式初始化 */
void init_modern_styles(void);

/* 调试函数 */
void enable_debug_mode(void);

#ifdef __cplusplus
}
#endif

#endif /* __AI_ASSISTANT_UI_H */