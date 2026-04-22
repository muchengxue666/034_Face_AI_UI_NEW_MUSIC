/**
 ******************************************************************************
 * @file        mipi_cam.c
 * @version     V1.1 (鏃犳劅鍚庡彴 AI 鐗?
 * @brief       mipicamera椹卞姩浠ｇ爜 - 宸插墺绂诲睆骞曟帹娴侊紝涓撲緵 AI 瑙嗚
 ******************************************************************************
 * @attention   Waiken-Smart 鎱у嫟鏅鸿繙
 * * 瀹為獙骞冲彴:     鎱у嫟鏅鸿繙 ESP32-P4 寮€鍙戞澘
 ******************************************************************************
 */

#include "mipi_cam.h"
#include "mipi_lcd.h"
#include "face_ai.h"
#include "gesture_ai.h"
#include "ai_assistant_ui.h"

static const char *mipi_cam_tag = "mipi_cam";

#define FPS_FUNC_ON   0         /* 帧率日志默认关闭，避免长期刷日志影响性能 */
#define ALIGN_UP_BY(num, align) (((num) + ((align) - 1)) & ~((align) - 1))  /* 瀵归綈鎿嶄綔 */

static void lcd_cam_task(void *arg);


#define GESTURE_REPORT_INTERVAL_US   (800 * 1000)
#define FACE_AI_INTERVAL_US           (120 * 1000)
#define GESTURE_AI_INTERVAL_US        (120 * 1000)
#define FACE_AI_INTERVAL_IDLE_US      (300 * 1000)
#define GESTURE_AI_INTERVAL_IDLE_US   (300 * 1000)

static volatile bool s_camera_pause_requested = false;
static volatile bool s_camera_streaming = true;

/**
 * @brief       mipi_cam鍒濆鍖?
 * @param       鏃?
 * @retval      ESP_OK:寮€鍚垚鍔? ESP_FAIL:寮€鍚け璐?
 */
esp_err_t mipi_cam_init(void)
{
    esp_err_t ret = ESP_OK;
    mipi_dev_bsp_enable_dsi_phy_power();    /* 閰嶇疆MIPI DSI PHY鐢垫簮锛堟憚鍍忓ご闇€瑕侊級*/

    ret = app_video_main(my_i2c_bus_handle);       /* 鍒濆鍖栨憚鍍忓ご纭欢鍜岃蒋浠舵帴鍙?*/
    if (ret != ESP_OK)
    {
        ESP_LOGE(mipi_cam_tag, "video main init failed with error 0x%x", ret);
        return ESP_FAIL;
    }

    int video_cam_fd0 = app_video_open(0);
    if (video_cam_fd0 < 0)
    {
        ESP_LOGE(mipi_cam_tag, "video cam open failed");
        return ESP_FAIL;
    }

    ret = app_video_init(video_cam_fd0, APP_VIDEO_FMT_RGB565);
    if (ret != ESP_OK)
    {
        ESP_LOGE(mipi_cam_tag, "Video cam init failed with error 0x%x", ret);
        return ESP_FAIL;
    }

    xTaskCreatePinnedToCore(lcd_cam_task, "lcd cam display", 4096, &video_cam_fd0, 2, NULL, 0);

    return ESP_OK;
}

esp_err_t mipi_cam_set_paused(bool paused)
{
    s_camera_pause_requested = paused;
    ESP_LOGI(mipi_cam_tag, "Camera pause request: %s", paused ? "pause" : "resume");
    return ESP_OK;
}

bool mipi_cam_is_paused(void)
{
    return s_camera_pause_requested;
}

/**
 * @brief       鎽勫儚澶撮噰闆嗕笌 AI 璇嗗埆浠诲姟 (绾悗鍙帮紝涓嶄笂灞?
 * @param       arg: 浼犲叆鍙傛暟(瑙嗛鎻忚堪绗?
 * @retval      鏃?
 */
static void lcd_cam_task(void *arg)
{
    int video_fd = *((int *)arg);
    int64_t last_gesture_report_us = 0;
    int64_t last_face_ai_us = 0;
    int64_t last_gesture_ai_us = 0;

    int res = 0;
    struct v4l2_buffer buf;             /* 瑙嗛缂撳啿淇℃伅 */

    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    struct v4l2_format format = {0};    /* 閺佺増宓佸ù浣圭壐瀵?*/
    format.type = type;/* CAM BUF璁剧疆涓や釜,浣跨敤MMAP */
    void *camera_outbuf[2];
    ESP_ERROR_CHECK(camera_set_bufs(video_fd, 2, NULL));    /* 璁剧疆CAMERA鐨勫抚缂撳瓨鏁伴噺 */
    ESP_ERROR_CHECK(camera_get_bufs(2, &camera_outbuf[0])); /* 鑾峰彇CAMERA鐨勫抚缂撳啿 */

    ESP_ERROR_CHECK(camera_stream_start(video_fd));         /* 寮€鍚棰戞祦 */

#if FPS_FUNC_ON == 1                                        /* FPS_FUNC_ON璇ュ畯 鎵撳紑甯х巼鏄剧ず鍔熻兘 */
    int fps_count = 0;
    int64_t start_time = esp_timer_get_time();
#endif

    if (ioctl(video_fd, VIDIOC_G_FMT, &format) != 0)        /* 鑾峰彇璁剧疆鏀寔鐨勮棰戞牸寮?*/
    {
        ESP_LOGE(mipi_cam_tag, "get fmt failed");
    }

    while (1)
    {
#if FPS_FUNC_ON == 1                                    /* FPS_FUNC_ON璇ュ畯 鎵撳紑甯х巼鏄剧ず鍔熻兘 */
        if (s_camera_pause_requested) {
            if (s_camera_streaming) {
                if (camera_stream_stop(video_fd) == ESP_OK) {
                    s_camera_streaming = false;
                    ESP_LOGI(mipi_cam_tag, "Camera stream paused");
                }
            }
            vTaskDelay(pdMS_TO_TICKS(30));
            continue;
        } else if (!s_camera_streaming) {
            if (camera_stream_start(video_fd) == ESP_OK) {
                s_camera_streaming = true;
                ESP_LOGI(mipi_cam_tag, "Camera stream resumed");
            }
            vTaskDelay(pdMS_TO_TICKS(30));
            continue;
        }

        fps_count++;
        if (fps_count == 50) {
            int64_t end_time = esp_timer_get_time();
            ESP_LOGI(mipi_cam_tag, "fps: %f", 1000000.0 / ((end_time - start_time) / 50.0));
            start_time = end_time;
            fps_count = 0;
        }
#endif
        memset(&buf, 0, sizeof(buf));
        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;       /* 涓庡墠闈ormat.type瑕佷竴鑷?*/
        buf.memory = V4L2_MEMORY_MMAP;                  /* 鍐呭瓨浣跨敤mmap鏂瑰紡 */
        
        res = ioctl(video_fd, VIDIOC_DQBUF, &buf);      /* 灏嗗凡缁忔崟鑾峰ソ瑙嗛鐨勫唴瀛樻媺鍑哄凡鎹曡幏瑙嗛鐨勯槦鍒?*/
        if (res != 0)
        {
            ESP_LOGE(mipi_cam_tag, "failed to receive video frame");
        }
        
        // ==========================================================
        // AI 瑙嗚鎷︽埅鐐?
        if (res == 0) {
            int64_t now_us = esp_timer_get_time();
            ui_state_t ui_state = g_ai_ui.current_state;
            bool active_interaction_state = (ui_state == UI_STATE_STANDBY)
                                         || (ui_state == UI_STATE_AVATAR)
                                         || (ui_state == UI_STATE_VIEWER)
                                         || (ui_state == UI_STATE_MUSIC);

            int64_t face_interval_us = active_interaction_state ? FACE_AI_INTERVAL_US : FACE_AI_INTERVAL_IDLE_US;
            int64_t gesture_interval_us = active_interaction_state ? GESTURE_AI_INTERVAL_US : GESTURE_AI_INTERVAL_IDLE_US;

            /* 推理降频：避免每帧都跑 AI 导致 UI 帧率被长期拖垮 */
            if ((now_us - last_face_ai_us) >= face_interval_us) {
                process_face_frame((uint8_t *)camera_outbuf[buf.index],
                                   format.fmt.pix.width,
                                   format.fmt.pix.height);
                last_face_ai_us = now_us;
            }

            if ((now_us - last_gesture_ai_us) >= gesture_interval_us) {
                gesture_result_t gesture_res;
                gesture_type_t g = process_gesture_frame(
                    (uint8_t *)camera_outbuf[buf.index],
                    format.fmt.pix.width,
                    format.fmt.pix.height,
                    &gesture_res);

                if (g != GESTURE_NONE) {
                    ESP_LOGD(mipi_cam_tag, "Hand gesture: %s (conf=%.2f) box=[%d,%d,%d,%d]",
                             gesture_type_to_str(g), gesture_res.confidence,
                             gesture_res.box_x1, gesture_res.box_y1,
                             gesture_res.box_x2, gesture_res.box_y2);

                    if ((now_us - last_gesture_report_us) >= GESTURE_REPORT_INTERVAL_US) {
                        last_gesture_report_us = now_us;
                        ui_handle_gesture_event(
                            g,
                            (int16_t)((gesture_res.box_x1 + gesture_res.box_x2) / 2),
                            (int16_t)((gesture_res.box_y1 + gesture_res.box_y2) / 2));
                    }
                }

                last_gesture_ai_us = now_us;
            }
        }
        // ==========================================================

        if (ioctl(video_fd, VIDIOC_QBUF, &buf) != 0)    /* 灏嗙┖闂茬殑鍐呭瓨鍔犲叆鍙崟鑾疯棰戠殑闃熷垪 */
        {
            ESP_LOGE(mipi_cam_tag, "failed to free video frame");
        }

        /* 主动让出 CPU，避免长期占满核心导致 UI 帧率周期性下降 */
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}


