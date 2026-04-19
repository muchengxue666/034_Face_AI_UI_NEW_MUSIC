#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 向后兼容LVGL 8
 */

typedef lv_disp_t lv_display_t;
/* 定义枚举类型lv_disp_rotation_t，用于表示显示设备的旋转角度 */
typedef enum {
    LV_DISPLAY_ROTATION_0 = LV_DISP_ROT_NONE,   /* 旋转角度为0 */
    LV_DISPLAY_ROTATION_90 = LV_DISP_ROT_90,    /* 旋转角度为90 */
    LV_DISPLAY_ROTATION_180 = LV_DISP_ROT_180,  /* 旋转角度为180 */
    LV_DISPLAY_ROTATION_270 = LV_DISP_ROT_270   /* 旋转角度为270 */
} lv_disp_rotation_t;
/* 定义lv_display_rotation_t类型，用于表示显示设备的旋转角度 */
typedef lv_disp_rotation_t lv_display_rotation_t;

#ifdef __cplusplus
}
#endif
