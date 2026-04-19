#pragma once

#include "esp_err.h"
#include "lvgl.h"

#if LVGL_VERSION_MAJOR == 8
#include "esp_lvgl_port_compatibility.h"  /* LVGL 8版本兼容性接口 */
#endif


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LVGL端口任务事件类型
 */
typedef enum {
    LVGL_PORT_EVENT_DISPLAY = 1,    /*  显示事件 */
    LVGL_PORT_EVENT_TOUCH   = 2,    /*  触摸事件 */
    LVGL_PORT_EVENT_USER    = 99,   /*  用户事件 */
} lvgl_port_event_type_t;

/**
 * @brief LVGL端口任务事件结构体
 */
typedef struct {
    lvgl_port_event_type_t type;    /*  事件类型 */
    void *param;                    /*  用户参数 */
} lvgl_port_event_t;

/**
 * @brief 初始化配置结构体
 */
typedef struct {
    int task_priority;      /*  LVGL任务优先级 */
    int task_stack;         /*  LVGL任务栈大小 */
    int task_affinity;      /*  LVGL任务绑定的核心 (-1表示不绑定核心) */
    int task_max_sleep_ms;  /*  LVGL任务最大休眠时间，单位为毫秒 */
    int timer_period_ms;    /*  LVGL定时器滴答周期，单位为毫秒 */
} lvgl_port_cfg_t;

/**
 * @brief LVGL端口初始化配置宏
 *
 */
#define ESP_LVGL_PORT_INIT_CONFIG() \
    {                               \
        .task_priority = 4,       \
        .task_stack = 6144,       \
        .task_affinity = -1,      \
        .task_max_sleep_ms = 500, \
        .timer_period_ms = 5,     \
    }

/**
 * @brief 初始化LVGL端口
 *
 * @note 此函数初始化LVGL，并创建定时器和任务以确保LVGL的正常工作。
 *
 * @return
 *      - ESP_OK                    成功
 *      - ESP_ERR_INVALID_ARG       如果创建参数无效
 *      - ESP_ERR_INVALID_STATE     如果esp_timer库尚未初始化
 *      - ESP_ERR_NO_MEM            如果内存分配失败
 */
esp_err_t lvgl_port_init(const lvgl_port_cfg_t *cfg);

/**
 * @brief 反初始化LVGL端口
 *
 * @note 此函数反初始化LVGL并停止正在运行的任务。
 * 在停止任务后，部分反初始化操作将继续进行。
 *
 * @return
 *      - ESP_OK                    成功
 *      - ESP_ERR_TIMEOUT           如果停止LVGL任务超时
 */
esp_err_t lvgl_port_deinit(void);

/**
 * @brief 获取LVGL互斥锁
 *
 * @param timeout_ms 超时时间，单位为毫秒。0表示无限期阻塞。
 * @return
 *      - true  成功获取互斥锁
 *      - false 获取互斥锁失败
 */
bool lvgl_port_lock(uint32_t timeout_ms);

/**
 * @brief 释放LVGL互斥锁
 *
 */
void lvgl_port_unlock(void);

/**
 * @brief 通知LVGL，数据已刷新到LCD显示器
 *
 * @note 该函数应仅在不调用LVGL端口内时使用（更多内容请见README）。
 *
 * @param disp          LVGL显示句柄（由lvgl_port_add_disp返回）
 */
void lvgl_port_flush_ready(lv_display_t *disp);

/**
 * @brief 停止LVGL定时器
 *
 * @return
 *      - ESP_OK 成功
 *      - ESP_ERR_INVALID_STATE 如果定时器未在运行
 */
esp_err_t lvgl_port_stop(void);

/**
 * @brief 恢复LVGL定时器
 *
 * @return
 *      - ESP_OK 成功
 *      - ESP_ERR_INVALID_STATE 如果定时器未在运行
 */
esp_err_t lvgl_port_resume(void);

/**
 * @brief 通知LVGL任务，显示器需要重新加载
 *
 * @note 该函数通常在LVGL事件和触摸中断中调用。
 *
 * @param event     事件类型
 * @param param     用户参数
 * @return
 *      - ESP_OK 成功
 *      - ESP_ERR_NOT_SUPPORTED 如果未实现
 *      - ESP_ERR_INVALID_STATE 如果队列未初始化（可能在LVGL反初始化后返回）
 */
esp_err_t lvgl_port_task_wake(lvgl_port_event_type_t event, void *param);

#ifdef __cplusplus
}
#endif
