/**
 ******************************************************************************
 * @file        lvgl_demo.c
 * @version     V1.0
 * @brief       LVGL V8绉绘 瀹為獙
 ******************************************************************************
 * @attention   Waiken-Smart 鎱у嫟鏅鸿繙
 * 
 * 瀹為獙骞冲彴:     鎱у嫟鏅鸿繙 ESP32-P4 寮€鍙戞澘
 ******************************************************************************
 */

#include "lvgl_demo.h"
#include "lcd.h"
#include "touch.h"
#include "esp_timer.h"
#include "lvgl.h"
#include "demos/lv_demos.h"
#include "esp_lvgl_port.h"
#include "esp_lvgl_port_disp.h"
#include "ai_assistant_ui.h"
#include "mqtt_app.h"
#include "control_panel.h"
#include "music_screen.h"
#include <stdio.h>


/**
 * @brief       鍒濆鍖栧苟娉ㄥ唽鏄剧ず璁惧
 * @param       鏃? * @retval      lvgl鏄剧ず璁惧鎸囬拡
 */
lv_disp_t *lv_port_disp_init(void)
{
    lv_disp_t *lcd_disp_handle = NULL; 

    /* 鍒濆鍖栨樉绀鸿澶嘗CD */
    lcd_init();                 /* LCD鍒濆鍖?*/

    if (lcddev.id <= 0x7084)    /* RGB灞忚Е鎽稿睆 */
    {
        /* 鍒濆鍖朙VGL鏄剧ず閰嶇疆 */
        const lvgl_port_display_cfg_t rgb_disp_cfg = {
            .panel_handle = lcddev.lcd_panel_handle,
            .buffer_size = lcddev.width * lcddev.width,
            .double_buffer = 0,
            .hres = lcddev.width,
            .vres = lcddev.height,
            .monochrome = false,
#if LVGL_VERSION_MAJOR >= 9
            .color_format = LV_COLOR_FORMAT_RGB565,
#endif
            .rotation = {           /* 蹇呴』涓嶳GBLCD閰嶇疆涓€鏍?*/
                .swap_xy = false,
                .mirror_x = false,
                .mirror_y = false,
            },
            .flags = {
                .buff_dma = false,
                .buff_spiram = false,
                .full_refresh = false,
                .direct_mode = true,
#if LVGL_VERSION_MAJOR >= 9
                .swap_bytes = false,
#endif
            }
        };
        const lvgl_port_display_rgb_cfg_t rgb_cfg = {
            .flags = {
                .bb_mode = true,
                .avoid_tearing = true,
            }
        };

        lcd_disp_handle = lvgl_port_add_disp_rgb(&rgb_disp_cfg, &rgb_cfg);
    }
    else                        /* MIPI灞忚Е鎽稿睆 */
    {
        /* 鍒濆鍖朙VGL鏄剧ず閰嶇疆 */
        const lvgl_port_display_cfg_t disp_cfg = {
            .io_handle = lcddev.lcd_dbi_io,         /* 璁剧疆io_handle涓簂cddev.lcd_dbi_io锛岀敤浜庡鐞嗘樉绀鸿澶囩殑IO鎿嶄綔 */
            .panel_handle = lcddev.lcd_panel_handle,/* 璁剧疆panel_handle涓簂cddev.lcd_panel_handle锛岀敤浜庡鐞嗘樉绀鸿澶囩殑闈㈡澘鎿嶄綔 */
            .control_handle = NULL,                 /* 璁剧疆control_handle涓篘ULL锛岀敤浜庡鐞嗘樉绀鸿澶囩殑鎺у埗鎿嶄綔 */
            .buffer_size = lcddev.width * lcddev.height,    /* 璁剧疆buffer_size涓簂cddev.height * lcddev.height锛岀敤浜庤缃樉绀鸿澶囩殑缂撳啿鍖哄ぇ灏?*/
            .double_buffer = true,                 /* 璁剧疆double_buffer涓篺alse锛岀敤浜庤缃樉绀鸿澶囨槸鍚︿娇鐢ㄥ弻缂撳啿 */
            .hres = lcddev.width,                   /* 璁剧疆hres涓簂cddev.width锛岀敤浜庤缃樉绀鸿澶囩殑姘村钩鍒嗚鲸鐜?*/
            .vres = lcddev.height,                  /* 璁剧疆vres涓簂cddev.height锛岀敤浜庤缃樉绀鸿澶囩殑鍨傜洿鍒嗚鲸鐜?*/
            .monochrome = false,                    /* 璁剧疆monochrome涓篺alse锛岀敤浜庤缃樉绀鸿澶囨槸鍚︿负鍗曡壊 */
            .rotation = {                           /* 鏃嬭浆鍊煎繀椤讳笌esp_lcd涓敤浜庡睆骞曞垵濮嬭缃殑鍊肩浉鍚?*/
                .swap_xy = false,
                .mirror_x = true,
                .mirror_y = false,
            },
#if LVGL_VERSION_MAJOR >= 9                     /* LVGL9锛?*/
#if CONFIG_BSP_LCD_COLOR_FORMAT_RGB888
            .color_format = LV_COLOR_FORMAT_RGB888,
#else
            .color_format = LV_COLOR_FORMAT_RGB565,
#endif
#endif
            .flags = {
                .buff_dma = false,                 /* 鍒嗛厤鐨凩VGL缂撳啿鍖烘敮鎸丏MA */
                .buff_spiram = true,               /* 鍒嗛厤鐨凩VGL缂撳啿鍖轰娇鐢≒SRAM */
#if LVGL_VERSION_MAJOR >= 9
                .swap_bytes = (BSP_LCD_BIGENDIAN ? true : false),
#endif
                .sw_rotate = false,                 /* SW鏃嬭浆涓嶆敮鎸侀伩鍏嶆挄瑁?*/
                .full_refresh = false,              /* 鍏ㄥ睆鍒锋柊 */
                .direct_mode = true,                /* 浣跨敤灞忓箷澶у皬鐨勭紦鍐插尯骞剁粯鍒跺埌缁濆鍧愭爣 */
            }
        };

        const lvgl_port_display_dsi_cfg_t  dpi_cfg = {
            .flags = {
                .avoid_tearing = true,              /* 寮€鍚槻鎾曡 */
            }
        };

        lcd_disp_handle = lvgl_port_add_disp_dsi(&disp_cfg, &dpi_cfg);
    }

    return lcd_disp_handle;                    
}

/**
 * @brief       鍥惧舰搴撶殑瑙︽懜灞忚鍙栧洖璋冨嚱鏁? * @param       indev_drv   : 瑙︽懜灞忚澶? * @param       data        : 杈撳叆璁惧鏁版嵁缁撴瀯浣? * @retval      鏃? */
void touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    assert(indev_drv); /* 纭繚椹卞姩绋嬪簭鏈夋晥 */
    /* 浠庤Е鎽告帶鍒跺櫒璇诲彇鏁版嵁鍒板唴瀛?*/
    tp_dev.scan(0); /* 鎵弿瑙︽懜鏁版嵁 */

    if (tp_dev.sta & TP_PRES_DOWN) /* 妫€鏌ヨЕ鎽告槸鍚︽寜涓?*/
    {
        data->point.x = tp_dev.x[0]; /* 鑾峰彇瑙︽懜鐐筙鍧愭爣 */
        data->point.y = tp_dev.y[0]; /* 鑾峰彇瑙︽懜鐐筜鍧愭爣 */
        data->state = LV_INDEV_STATE_PRESSED; /* 璁剧疆鐘舵€佷负鎸変笅 */
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED; /* 璁剧疆鐘舵€佷负閲婃斁 */
    }
}

/**
 * @brief       鍒濆鍖栧苟娉ㄥ唽杈撳叆璁惧
 * @param       鏃? * @retval      lvgl杈撳叆璁惧鎸囬拡
 */
lv_indev_t *lv_port_indev_init(void)
{
    /* 鍒濆鍖栬Е鎽稿睆 */
    tp_dev.init();

    /* 鍒濆鍖栬緭鍏ヨ澶?*/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);

    /* 閰嶇疆杈撳叆璁惧绫诲瀷 */
    indev_drv.type = LV_INDEV_TYPE_POINTER;

    /* 璁剧疆杈撳叆璁惧璇诲彇鍥炶皟鍑芥暟 */
    indev_drv.read_cb = touchpad_read;

    /* 鍦↙VGL涓敞鍐岄┍鍔ㄧ▼搴忥紝骞朵繚瀛樺垱寤虹殑杈撳叆璁惧瀵硅薄 */
    return lv_indev_drv_register(&indev_drv);
}

/**
 * @brief       lvgl_demo鍏ュ彛鍑芥暟
 * @param       鏃? * @retval      鏃? */
void lvgl_demo(void)
{
    lv_disp_t *disp = NULL;
    lv_indev_t *indev = NULL;

    /* 鍒濆鍖朙VGL绔彛閰嶇疆 */
    lvgl_port_cfg_t lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    /* 鍒濆鍖朙VGL绔彛 */
    lvgl_port_init(&lvgl_port_cfg);

    /*
     * 鍦ㄥ垵濮嬪寲鏄剧ず/杈撳叆椹卞姩鏈熼棿鎸佹湁 LVGL 閫掑綊閿侊紝閬垮厤 LVGL 浠诲姟骞跺彂杩涘叆鍒锋柊娴佺▼銆?     * 杩欒兘瑙勯伩鏄剧ず娉ㄥ唽涓庨甯у埛鏂颁箣闂寸殑鏃跺簭绔炴€併€?     */
    if (!lvgl_port_lock(1000)) {
        ESP_LOGE("lvgl_demo", "Failed to lock LVGL during display init");
        return;
    }

    disp = lv_port_disp_init();    /* lvgl鏄剧ず鎺ュ彛鍒濆鍖?鏀惧湪lv_init()鐨勫悗闈?*/
    indev = lv_port_indev_init();  /* lvgl杈撳叆鎺ュ彛鍒濆鍖?鏀惧湪lv_init()鐨勫悗闈?*/

    lvgl_port_unlock();

    if (disp == NULL || indev == NULL) {
        ESP_LOGE("lvgl_demo", "Display/Input init failed: disp=%p, indev=%p", (void *)disp, (void *)indev);
        return;
    }

    /* 绠€鍗曠殑绾壊娴嬭瘯 - 妫€鏌CD鏄惁姝ｅ父宸ヤ綔 */
    // ESP_LOGI("lvgl_demo", "Starting RED color test for 2 seconds...");
    // if (lvgl_port_lock(0)) {
    //     lv_obj_t *screen = lv_scr_act();
    //     /* 灏濊瘯璁剧疆涓虹孩鑹?*/
    //     lv_obj_set_style_bg_color(screen, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
    //     lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    //     lvgl_port_unlock();
    // }

    // /* 绛夊緟2绉掓樉绀虹孩鑹?*/
    // for (int i = 0; i < 20; i++) {
    //     vTaskDelay(pdMS_TO_TICKS(100));
    // }

    // ESP_LOGI("lvgl_demo", "RED color test complete, initializing AI Assistant UI...");

    /* 鍚姩AI鍔╂墜UI妗嗘灦 - 瀹冧細鑷繁澶勭悊閿佸畾 */
    ai_assistant_ui_init();

    ESP_LOGI("lvgl_demo", "AI Assistant UI initialized, entering main loop...");

    ui_msg_t new_note;
    gesture_msg_t gesture_msg;

    /* 涓诲惊鐜?- 澶勭悊 MQTT 渚跨娑堟伅 */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));

        if (ui_msg_queue != NULL && xQueueReceive(ui_msg_queue, &new_note, 0) == pdPASS) {
            ESP_LOGI("lvgl_demo", "MQTT note received, updating UI...");
            if (lvgl_port_lock(pdMS_TO_TICKS(200))) {
                if (g_ai_ui.current_state != UI_STATE_CONTROL) {
                    if (!g_ai_ui.control_panel) {
                        g_ai_ui.control_panel = create_control_panel();
                    }
                    g_ai_ui.current_screen = g_ai_ui.control_panel;
                    g_ai_ui.current_state = UI_STATE_CONTROL;
                    lv_scr_load_anim(g_ai_ui.control_panel,
                                     LV_SCR_LOAD_ANIM_FADE_ON,
                                     400, 0, false);
                }
                sticky_note_add_message(new_note.sender, new_note.message, new_note.time);
                lvgl_port_unlock();
            }
        }

        while (g_ai_ui.gesture_queue != NULL &&
               xQueueReceive(g_ai_ui.gesture_queue, &gesture_msg, 0) == pdPASS) {
            ESP_LOGI("lvgl_demo", "Gesture received: %s at (%d,%d)",
                     gesture_type_to_str(gesture_msg.gesture), gesture_msg.x, gesture_msg.y);

            bool paused = false;
            if (lvgl_port_lock(pdMS_TO_TICKS(200))) {
                if (g_ai_ui.current_state == UI_STATE_MUSIC && music_screen_is_playing()) {
                    music_screen_pause_playback(gesture_type_to_str(gesture_msg.gesture));
                    paused = true;
                }
                lvgl_port_unlock();
            }

            if (paused) {
                char feedback[96];
                snprintf(feedback, sizeof(feedback), "检测到手势 %s,已停止播放",
                         gesture_type_to_str(gesture_msg.gesture));
                ui_show_gesture_feedback(feedback);
            }
        }
    }
}
