/**
 ******************************************************************************
 * @file        ai_assistant_ui.c
 * @version     V1.0
 * @brief       AI助手现代化UI框架 - 主实现文件
 ******************************************************************************
 * @attention   基于LVGL v8.3+的现代化卡片式UI框架
 *              包含四个核心界面的管理与切换逻辑
 ******************************************************************************
 */

#include "ai_assistant_ui.h"
#include "standby_screen.h"
#include "home_screen.h"
#include "avatar_screen.h"      /* 新增：小柚子化身界面 */
#include "music_screen.h"       /* 新增：音乐播放界面 */
#include "control_panel.h"
#include "viewer_screen.h"
#include "ui_theme.h"           /* 统一主题定义 */
#include "esp_lvgl_port.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "lcd.h"  // 添加lcd.h以访问lcddev

/* 使用 LVGL 内置 CJK 字体替代自定义字体 */

static const char *TAG = "ai_ui";

/* 全局UI句柄定义 */
ai_ui_handle_t g_ai_ui = {0};

/* 调试模式标志 */
static bool s_debug_mode = false;

/* 函数前置声明 */
static void debug_color_test_cb(lv_event_t *e);
static void diag_test_cb(lv_event_t *e);

/**
 * @brief       显示诊断测试
 * @param       无
 * @retval      无
 */
static void display_diagnostic_test(void)
{
    ESP_LOGI(TAG, "Starting display diagnostic test...");
    
    /* 测试1: 清屏为纯色 */
    ESP_LOGI(TAG, "Test 1: Pure color screen");
    if (lvgl_port_lock(0)) {
        lv_obj_t *screen = lv_scr_act();
        lv_obj_set_style_bg_color(screen, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
        lvgl_port_unlock();
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    if (lvgl_port_lock(0)) {
        lv_obj_t *screen = lv_scr_act();
        lv_obj_set_style_bg_color(screen, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);
        lvgl_port_unlock();
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    if (lvgl_port_lock(0)) {
        lv_obj_t *screen = lv_scr_act();
        lv_obj_set_style_bg_color(screen, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN);
        lvgl_port_unlock();
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    /* 测试2: 检查LVGL渲染 */
    ESP_LOGI(TAG, "Test 2: LVGL rendering test");
    if (lvgl_port_lock(0)) {
        lv_obj_clean(lv_scr_act());
        
        lv_obj_t *label = lv_label_create(lv_scr_act());
        lv_label_set_text(label, "LVGL 渲染测试");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_text_font(label, THEME_FONT_CN, LV_PART_MAIN);
        lv_obj_set_style_text_color(label, THEME_TEXT_ON_DARK, LV_PART_MAIN);
        
        lvgl_port_unlock();
    }
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    ESP_LOGI(TAG, "Display diagnostic test completed");
}

/**
 * @brief       诊断测试按钮回调
 * @param       e: 事件句柄
 * @retval      无
 */
static void diag_test_cb(lv_event_t *e)
{
    display_diagnostic_test();
}

/**
 * @brief       调试模式颜色测试回调
 * @param       e: 事件句柄
 * @retval      无
 */
static void debug_color_test_cb(lv_event_t *e)
{
    /* 测试颜色显示 */
    lv_obj_t *screen = lv_scr_act();
    static int color_idx = 0;
    lv_color_t colors[] = {lv_palette_main(LV_PALETTE_RED), lv_palette_main(LV_PALETTE_GREEN), 
                          lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_YELLOW), 
                          lv_color_black()};
    lv_obj_set_style_bg_color(screen, colors[color_idx++ % 5], LV_PART_MAIN);
}

/**
 * @brief       调试模式下的测试屏幕
 * @param       无
 * @retval      屏幕对象指针
 */
static lv_obj_t* create_debug_screen(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    /* 设置背景为暖深棕以便观察 */
    lv_obj_set_style_bg_color(screen, THEME_BG_DARK, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    /* 添加调试信息标签 */
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "调试模式 - AI助手界面");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_font(title, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);

    char info_text[256];
    sprintf(info_text, "屏幕: %dx%d\nLVGL: %d.%d.%d\nPSRAM: %s\n状态: %d\n\n如果您看到此文字,\nLCD ID: 0x%lX\n分辨率正常",
            LV_HOR_RES, LV_VER_RES,
            LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH,
            heap_caps_get_total_size(MALLOC_CAP_SPIRAM) > 0 ? "可用" : "不可用",
            g_ai_ui.current_state,
            (unsigned long)lcddev.id);

    lv_obj_t *info_label = lv_label_create(screen);
    lv_label_set_text(info_label, info_text);
    lv_obj_align(info_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(info_label, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(info_label, lv_color_white(), LV_PART_MAIN);
    lv_label_set_long_mode(info_label, LV_LABEL_LONG_WRAP);

    /* 添加诊断测试按钮 */
    lv_obj_t *diag_btn = lv_btn_create(screen);
    lv_obj_set_size(diag_btn, 180, 40);
    lv_obj_align(diag_btn, LV_ALIGN_BOTTOM_MID, 0, -60);
    lv_obj_t *diag_label = lv_label_create(diag_btn);
    lv_label_set_text(diag_label, "运行诊断");
    lv_obj_set_style_text_font(diag_label, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(diag_label, THEME_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_add_event_cb(diag_btn, diag_test_cb, LV_EVENT_CLICKED, NULL);

    /* 添加测试按钮 */
    lv_obj_t *test_btn = lv_btn_create(screen);
    lv_obj_set_size(test_btn, 150, 40);
    lv_obj_align(test_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_t *btn_label = lv_label_create(test_btn);
    lv_label_set_text(btn_label, "测试颜色");
    lv_obj_set_style_text_font(btn_label, THEME_FONT_CN, LV_PART_MAIN);
    lv_obj_set_style_text_color(btn_label, THEME_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_add_event_cb(test_btn, debug_color_test_cb, LV_EVENT_CLICKED, NULL);

    /* 加载屏幕使其可见 */
    lv_scr_load(screen);

    return screen;
}

/**
 * @brief       检查显示硬件状态
 * @param       无
 * @retval      true: 正常, false: 异常
 */
static bool check_display_hardware(void)
{
    ESP_LOGI(TAG, "Checking display hardware...");
    
    /* 检查LCD设备ID - 使用正确的格式化字符串 */
    ESP_LOGI(TAG, "LCD Device ID: 0x%08lX", (unsigned long)lcddev.id);
    ESP_LOGI(TAG, "LCD Resolution: %lux%lu", (unsigned long)lcddev.width, (unsigned long)lcddev.height);
    
    if (lcddev.id == 0 || lcddev.id == 0xFFFF) {
        ESP_LOGE(TAG, "Invalid LCD device ID!");
        return false;
    }
    
    if (lcddev.width == 0 || lcddev.height == 0) {
        ESP_LOGE(TAG, "Invalid LCD resolution!");
        return false;
    }
    
    return true;
}

/**
 * @brief       启用调试模式
 * @param       无
 * @retval      无
 */
void enable_debug_mode(void)
{
    s_debug_mode = true;
    ESP_LOGW(TAG, "Debug mode enabled manually!");
}

/**
 * @brief       初始化AI助手UI框架
 * @param       无
 * @retval      无
 */
void ai_assistant_ui_init(void)
{
    ESP_LOGI(TAG, "Initializing AI Assistant UI Framework...");
    
    /* 检查强制调试模式 */
    if (FORCE_DEBUG_MODE) {
        s_debug_mode = true;
        ESP_LOGW(TAG, "Force debug mode enabled by compile flag!");
    }
    
    /* 检查显示硬件 */
    if (!check_display_hardware()) {
        ESP_LOGE(TAG, "Display hardware check failed! Entering debug mode...");
        s_debug_mode = true;
    }
    
    /* 创建消息队列 */
    g_ai_ui.text_queue = xQueueCreate(64, sizeof(stream_text_msg_t));
    g_ai_ui.hardware_queue = xQueueCreate(10, sizeof(hardware_control_msg_t));
    g_ai_ui.gesture_queue = xQueueCreate(10, sizeof(gesture_msg_t));
    
    if (!g_ai_ui.text_queue || !g_ai_ui.hardware_queue || !g_ai_ui.gesture_queue) {
        ESP_LOGE(TAG, "Failed to create message queues!");
        s_debug_mode = true;
    }
    
    /* 创建UI互斥锁 */
    g_ai_ui.ui_mutex = xSemaphoreCreateMutex();
    if (!g_ai_ui.ui_mutex) {
        ESP_LOGE(TAG, "Failed to create UI mutex!");
        s_debug_mode = true;
    }
    
    /* 初始化现代化样式 */
    init_modern_styles();

    /* 根据调试模式选择初始屏幕 */
    if (s_debug_mode) {
        ESP_LOGW(TAG, "Entering DEBUG MODE due to initialization issues");
        if (lvgl_port_lock(0)) {
            ESP_LOGI(TAG, "Creating debug screen...");
            g_ai_ui.current_screen = create_debug_screen();
            g_ai_ui.current_state = UI_STATE_STANDBY; // 使用备用状态
            ESP_LOGI(TAG, "Debug screen created successfully");

            /* 强制刷新屏幕 */
            lv_refr_now(NULL);

            lvgl_port_unlock();
        } else {
            ESP_LOGE(TAG, "Failed to lock LVGL port for debug screen!");
        }
    } else {
        /* 默认进入多模态交互主页 - 而不是息屏唤醒状态 */
        ESP_LOGI(TAG, "Entering normal mode, switching to HOME screen...");
        ui_switch_to_standby();
    }

    ESP_LOGI(TAG, "AI Assistant UI Framework initialized successfully!");
}

/**
 * @brief       释放AI助手UI框架资源
 * @param       无
 * @retval      无
 */
void ai_assistant_ui_deinit(void)
{
    if (g_ai_ui.text_queue) {
        vQueueDelete(g_ai_ui.text_queue);
        g_ai_ui.text_queue = NULL;
    }
    if (g_ai_ui.hardware_queue) {
        vQueueDelete(g_ai_ui.hardware_queue);
        g_ai_ui.hardware_queue = NULL;
    }
    if (g_ai_ui.gesture_queue) {
        vQueueDelete(g_ai_ui.gesture_queue);
        g_ai_ui.gesture_queue = NULL;
    }
    if (g_ai_ui.ui_mutex) {
        vSemaphoreDelete(g_ai_ui.ui_mutex);
        g_ai_ui.ui_mutex = NULL;
    }
    /* 音乐界面单独释放（其余界面随屏幕对象树一起销毁）*/
    if (g_ai_ui.music_screen) {
        delete_music_screen();
        g_ai_ui.music_screen = NULL;
    }
}

/**
 * @brief       切换到息屏唤醒页
 * @param       无
 * @retval      无
 */
void ui_switch_to_standby(void)
{
    ESP_LOGI(TAG, "ui_switch_to_standby() called, current_state=%d", g_ai_ui.current_state);
    
    /* 如果已经在待机状态且屏幕已创建，则检查是否需要重新启动动画 */
    if (g_ai_ui.current_state == UI_STATE_STANDBY && g_ai_ui.standby_screen != NULL) {
        ESP_LOGI(TAG, "Already in standby state with screen created, checking animations");
        /* 屏幕已存在，只需重新启动动画即可 */
        if (lvgl_port_lock(pdMS_TO_TICKS(500))) {
            restart_breathing_animations();
            lvgl_port_unlock();
        }
        return;
    }
    
    ESP_LOGI(TAG, "Attempting to lock LVGL port...");
    /* 锁定UI互斥锁 - 增加超时时间确保初始化阶段也能成功 */
    if (lvgl_port_lock(pdMS_TO_TICKS(1000))) {
        ESP_LOGI(TAG, "LVGL port locked successfully");
        
        /* 创建或复用息屏唤醒页 */
        if (!g_ai_ui.standby_screen) {
            ESP_LOGI(TAG, "Creating new standby screen during initialization");
            g_ai_ui.standby_screen = create_standby_screen();
            if (g_ai_ui.standby_screen) {
                ESP_LOGI(TAG, "Standby screen created successfully");
            } else {
                ESP_LOGE(TAG, "FAILED: create_standby_screen() returned NULL");
                lvgl_port_unlock();
                return;
            }
        } else {
            ESP_LOGI(TAG, "Reusing existing standby screen");
            /* 重新应用背景样式 */
            lv_obj_set_style_bg_color(g_ai_ui.standby_screen, THEME_BG_DARK, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(g_ai_ui.standby_screen, LV_OPA_COVER, LV_PART_MAIN);
            /* 重新启动呼吸动画 */
            restart_breathing_animations();
            ESP_LOGI(TAG, "Standby screen reused and animations restarted");
        }

        g_ai_ui.current_screen = g_ai_ui.standby_screen;
        g_ai_ui.current_state = UI_STATE_STANDBY;

        /* 使用淡入淡出动画加载屏幕（过渡更丝滑）*/
        lv_scr_load_anim(g_ai_ui.standby_screen,
                         LV_SCR_LOAD_ANIM_FADE_ON,
                         THEME_SCREEN_ANIM_TIME,
                         0,      /* 无延迟 */
                         false   /* 不自动删除旧屏幕，保留缓存复用 */
        );
        ESP_LOGI(TAG, "Standby screen loaded with slide animation");

        lvgl_port_unlock();
        ESP_LOGI(TAG, "LVGL port unlocked");
    } else {
        ESP_LOGE(TAG, "FAILED: Could not lock LVGL port for standby screen switch!");
    }
}

/**
 * @brief       切换到多模态交互主页
 * @param       无
 * @retval      无
 */
void ui_switch_to_home(void)
{
    if (g_ai_ui.current_state == UI_STATE_HOME) {
        return;
    }

    /* 从 avatar / standby 过来 → 淡入；从深层页返回 → 淡入 */
    lv_scr_load_anim_t anim = LV_SCR_LOAD_ANIM_FADE_ON;

    if (lvgl_port_lock(0)) {
        if (!g_ai_ui.home_screen) {
            g_ai_ui.home_screen = create_home_screen();
        }
        /* 不删 avatar screen，只停止其内部动画，避免 layout/anim 竞争崩溃
         * avatar screen 对象保留在内存，下次唤醒时直接复用或重建 */
        if (g_ai_ui.avatar_screen) {
            stop_avatar_animations();
        }
        g_ai_ui.current_screen = g_ai_ui.home_screen;
        g_ai_ui.current_state  = UI_STATE_HOME;
        lv_scr_load_anim(g_ai_ui.home_screen, anim, THEME_SCREEN_ANIM_TIME, 0, false);
        lvgl_port_unlock();
    }
}

/**
 * @brief       切换到硬件控制抽屉
 * @param       无
 * @retval      无
 */
void ui_switch_to_control(void)
{
    if (g_ai_ui.current_state == UI_STATE_CONTROL) {
        return;
    }

    if (lvgl_port_lock(0)) {
        if (!g_ai_ui.control_panel) {
            g_ai_ui.control_panel = create_control_panel();
        }
        g_ai_ui.current_screen = g_ai_ui.control_panel;
        g_ai_ui.current_state  = UI_STATE_CONTROL;
        /* 便签淡入 */
        lv_scr_load_anim(g_ai_ui.control_panel,
                         LV_SCR_LOAD_ANIM_FADE_ON,
                         THEME_SCREEN_ANIM_TIME, 0, false);
        lvgl_port_unlock();
    }
}

/**
 * @brief       切换到知识库与影像视图
 * @param       无
 * @retval      无
 */
void ui_switch_to_viewer(void)
{
    if (g_ai_ui.current_state == UI_STATE_VIEWER) {
        return;
    }

    if (lvgl_port_lock(0)) {
        if (!g_ai_ui.viewer_screen) {
            g_ai_ui.viewer_screen = create_viewer_screen();
        }
        g_ai_ui.current_screen = g_ai_ui.viewer_screen;
        g_ai_ui.current_state  = UI_STATE_VIEWER;
        lv_scr_load_anim(g_ai_ui.viewer_screen,
                         LV_SCR_LOAD_ANIM_FADE_ON,
                         THEME_SCREEN_ANIM_TIME, 0, false);
        lvgl_port_unlock();
    }
}

/**
 * @brief       切换到小柚子化身界面
 * @param       无
 * @retval      无
 */
void ui_switch_to_avatar(void)
{
    ESP_LOGI(TAG, "Switching to avatar screen...");

    if (g_ai_ui.current_state == UI_STATE_AVATAR) {
        return;
    }

    if (lvgl_port_lock(pdMS_TO_TICKS(500))) {
        if (!g_ai_ui.avatar_screen) {
            g_ai_ui.avatar_screen = create_avatar_screen();
        } else {
            /* 复用旧 screen，重启动画和自动跳转定时器 */
            restart_avatar_animations();
        }
        g_ai_ui.current_screen = g_ai_ui.avatar_screen;
        g_ai_ui.current_state  = UI_STATE_AVATAR;
        lv_scr_load_anim(g_ai_ui.avatar_screen,
                         LV_SCR_LOAD_ANIM_FADE_ON,
                         THEME_SCREEN_ANIM_TIME, 0, false);
        lvgl_port_unlock();
        ESP_LOGI(TAG, "Avatar screen loaded");
    }
}

/**
 * @brief       切换到音乐播放界面
 * @param       无
 * @retval      无
 */
void ui_switch_to_music(void)
{
    if (g_ai_ui.current_state == UI_STATE_MUSIC) {
        return;
    }

    ESP_LOGI(TAG, "Switching to music screen...");

    if (lvgl_port_lock(pdMS_TO_TICKS(500))) {
        if (!g_ai_ui.music_screen) {
            g_ai_ui.music_screen = create_music_screen();
        }
        g_ai_ui.current_screen = g_ai_ui.music_screen;
        g_ai_ui.current_state  = UI_STATE_MUSIC;
        /* 音乐页淡入 */
        lv_scr_load_anim(g_ai_ui.music_screen,
                         LV_SCR_LOAD_ANIM_FADE_ON,
                         THEME_SCREEN_ANIM_TIME, 0, false);
        lvgl_port_unlock();
        ESP_LOGI(TAG, "Music screen loaded");
    }
}

/**
 * @brief       触发唤醒动画并切换到化身界面
 * @param       无
 * @retval      无
 * @attention   修改后流程：standby -> avatar -> home
 */
void ui_trigger_wake_up(void)
{
    if (g_ai_ui.current_state != UI_STATE_STANDBY) {
        return;
    }

    ESP_LOGI(TAG, "Wake up triggered, switching to avatar screen...");
    /* 现在唤醒后先进入小柚子化身界面，而不是直接进入主页 */
    ui_switch_to_avatar();
}

/**
 * @brief       更新流式文本显示
 * @param       ch: 要显示的字符
 * @param       is_complete: 是否为完整消息结束
 * @retval      无
 */
void ui_update_streaming_text(char ch, bool is_complete)
{
    stream_text_msg_t msg = {0};
    msg.character = ch;
    msg.is_complete = is_complete;
    
    /* 发送到文本队列（非阻塞） */
    if (g_ai_ui.text_queue) {
        xQueueSend(g_ai_ui.text_queue, &msg, 0);
    }
}

/**
 * @brief       更新硬件状态显示
 * @param       brightness: 屏幕亮度 (0-100)
 * @param       fan_on: 排风扇开关状态
 * @retval      无
 */
void ui_update_hardware_status(uint8_t brightness, bool fan_on)
{
    hardware_control_msg_t msg = {0};
    msg.brightness = brightness;
    msg.fan_state = fan_on;
    
    /* 发送到硬件队列（非阻塞） */
    if (g_ai_ui.hardware_queue) {
        xQueueSend(g_ai_ui.hardware_queue, &msg, 0);
    }
}

/**
 * @brief       处理手势事件
 * @param       gesture: 手势类型
 * @param       x: 手势X坐标
 * @param       y: 手势Y坐标
 * @retval      无
 */
void ui_handle_gesture_event(gesture_type_t gesture, int16_t x, int16_t y)
{
    gesture_msg_t msg = {0};
    msg.gesture = gesture;
    msg.x = x;
    msg.y = y;
    
    /* 发送到手势队列（非阻塞） */
    if (g_ai_ui.gesture_queue) {
        xQueueSend(g_ai_ui.gesture_queue, &msg, 0);
    }
}

/**
 * @brief       初始化现代化UI样式
 * @param       无
 * @retval      无
 */
void init_modern_styles(void)
{
    /* 启用抗锯齿 */
    lv_theme_default_init(NULL, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
                         LV_THEME_DEFAULT_DARK, LV_FONT_DEFAULT);

    /* 设置全局默认样式 */
    static lv_style_t global_style;
    lv_style_init(&global_style);
    lv_style_set_radius(&global_style, 8);  // 圆角半径
    lv_style_set_border_width(&global_style, 0);
    lv_style_set_pad_all(&global_style, 8);

    /* 应用到所有对象 */
    lv_obj_add_style(lv_scr_act(), &global_style, 0);
}

/* ==================== 全局手势反馈指示器 ==================== */

/**
 * @brief       手势反馈自动隐藏定时器回调
 * @param       timer: 定时器句柄
 * @retval      无
 */
static void gesture_hide_timer_cb(lv_timer_t *timer)
{
    ui_hide_gesture_feedback();
}

/**
 * @brief       显示手势反馈提示
 * @param       text: 提示文本（如"◀ 切换到音乐"）
 * @retval      无
 * @attention   非侵入式设计，2秒后自动隐藏
 */
void ui_show_gesture_feedback(const char *text)
{
    ESP_LOGI(TAG, "Showing gesture feedback: %s", text);

    if (!lvgl_port_lock(pdMS_TO_TICKS(100))) {
        return;
    }

    /* 如果指示器不存在，创建它 */
    if (!g_ai_ui.gesture_feedback) {
        /* 创建全局叠加层（固定在屏幕顶部） */
        g_ai_ui.gesture_feedback = lv_obj_create(lv_layer_top());
        lv_obj_set_width(g_ai_ui.gesture_feedback, lv_pct(60));
        lv_obj_set_height(g_ai_ui.gesture_feedback, 50);
        lv_obj_align(g_ai_ui.gesture_feedback, LV_ALIGN_TOP_MID, 0, 20);

        /* 半透明暖深色背景 */
        lv_obj_set_style_bg_color(g_ai_ui.gesture_feedback, THEME_BG_DARK, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(g_ai_ui.gesture_feedback, LV_OPA_90, LV_PART_MAIN);
        lv_obj_set_style_radius(g_ai_ui.gesture_feedback, 25, LV_PART_MAIN);
        lv_obj_set_style_border_width(g_ai_ui.gesture_feedback, 2, LV_PART_MAIN);
        lv_obj_set_style_border_color(g_ai_ui.gesture_feedback, THEME_PRIMARY, LV_PART_MAIN);
        lv_obj_set_style_border_opa(g_ai_ui.gesture_feedback, LV_OPA_70, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(g_ai_ui.gesture_feedback, 15, LV_PART_MAIN);
        lv_obj_set_style_shadow_color(g_ai_ui.gesture_feedback, THEME_PRIMARY, LV_PART_MAIN);
        lv_obj_set_style_shadow_opa(g_ai_ui.gesture_feedback, LV_OPA_30, LV_PART_MAIN);

        /* 初始隐藏 */
        lv_obj_add_flag(g_ai_ui.gesture_feedback, LV_OBJ_FLAG_HIDDEN);

        /* 创建文本标签 */
        lv_obj_t *label = lv_label_create(g_ai_ui.gesture_feedback);
        lv_obj_set_style_text_font(label, THEME_FONT_CN, LV_PART_MAIN);
        lv_obj_set_style_text_color(label, THEME_TEXT_ON_DARK, LV_PART_MAIN);
        lv_obj_center(label);
    }

    /* 更新文本 */
    lv_obj_t *label = lv_obj_get_child(g_ai_ui.gesture_feedback, 0);
    if (label) {
        lv_label_set_text(label, text);
    }

    /* 显示指示器 */
    lv_obj_clear_flag(g_ai_ui.gesture_feedback, LV_OBJ_FLAG_HIDDEN);

    /* 滑入动画（从顶部滑入） */
    lv_anim_t slide_anim;
    lv_anim_init(&slide_anim);
    lv_anim_set_var(&slide_anim, g_ai_ui.gesture_feedback);
    lv_anim_set_values(&slide_anim, -60, 20);
    lv_anim_set_time(&slide_anim, 200);
    lv_anim_set_exec_cb(&slide_anim, (lv_anim_exec_xcb_t)(void *)lv_obj_set_y);
    lv_anim_set_path_cb(&slide_anim, lv_anim_path_ease_out);
    lv_anim_start(&slide_anim);

    /* 设置自动隐藏定时器（2秒后） */
    if (g_ai_ui.gesture_hide_timer) {
        lv_timer_del(g_ai_ui.gesture_hide_timer);
    }
    g_ai_ui.gesture_hide_timer = lv_timer_create(gesture_hide_timer_cb, 2000, NULL);
    lv_timer_set_repeat_count(g_ai_ui.gesture_hide_timer, 1);

    lvgl_port_unlock();
}

/**
 * @brief       隐藏手势反馈提示
 * @param       无
 * @retval      无
 */
void ui_hide_gesture_feedback(void)
{
    if (!g_ai_ui.gesture_feedback) {
        return;
    }

    if (!lvgl_port_lock(pdMS_TO_TICKS(100))) {
        return;
    }

    /* 滑出动画（向上滑出） */
    lv_anim_t slide_anim;
    lv_anim_init(&slide_anim);
    lv_anim_set_var(&slide_anim, g_ai_ui.gesture_feedback);
    lv_anim_set_values(&slide_anim, 20, -60);
    lv_anim_set_time(&slide_anim, 200);
    lv_anim_set_exec_cb(&slide_anim, (lv_anim_exec_xcb_t)(void *)lv_obj_set_y);
    lv_anim_set_path_cb(&slide_anim, lv_anim_path_ease_in);
    lv_anim_start(&slide_anim);

    /* 延迟隐藏 */
    lv_obj_add_flag(g_ai_ui.gesture_feedback, LV_OBJ_FLAG_HIDDEN);

    /* 清理定时器 */
    if (g_ai_ui.gesture_hide_timer) {
        lv_timer_del(g_ai_ui.gesture_hide_timer);
        g_ai_ui.gesture_hide_timer = NULL;
    }

    lvgl_port_unlock();
    ESP_LOGI(TAG, "Gesture feedback hidden");
}