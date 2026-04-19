#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 显示设备类型的配置枚举
 */
typedef enum {
    LVGL_PORT_DISP_TYPE_OTHER, /* 其他显示类型 */
    LVGL_PORT_DISP_TYPE_DSI,   /* 使用DSI接口的显示设备 */
    LVGL_PORT_DISP_TYPE_RGB,   /* 使用RGB接口的显示设备 */
} lvgl_port_disp_type_t;

/**
 * @brief 显示设备私有配置结构体
 */
typedef struct {
    unsigned int avoid_tearing: 1; /* 使用内部RGB缓冲区作为LVGL绘图缓冲区，以避免屏幕撕裂效果 */
} lvgl_port_disp_priv_cfg_t;

/**
 * @brief 通知LVGL任务
 *
 * @note 此函数通常在RGB显示同步信号（vsync ready）触发时调用
 *
 * @param value     通知的值
 * @return
 *      - true  是否有高优先级任务被唤醒
 */
bool lvgl_port_task_notify(uint32_t value);

#ifdef __cplusplus
}
#endif
