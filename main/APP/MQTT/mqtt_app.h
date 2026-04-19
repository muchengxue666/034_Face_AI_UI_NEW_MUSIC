/**
 ******************************************************************************
 * @file        mqtt_app.h
 * @version     V2.0
 * @brief       MQTT 通信与 UI 消息队列头文件
 ******************************************************************************
 */

#ifndef _MQTT_APP_H_
#define _MQTT_APP_H_

#ifdef __cplusplus
extern "C" {
#endif

// 引入 FreeRTOS 队列所需的头文件
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/* ==================== 数据结构定义 ==================== */

// 定义 UI 消息结构体：专门用来给 LVGL 传便签数据
// 这里的数组大小要和 mqtt_app.c 以及队友的 UI 需求匹配
typedef struct {
    char sender[64];      // 发送者（例如：女儿、老伴）
    char message[256];    // 留言内容
    char time[32];        // 时间戳（例如：刚刚）
} ui_msg_t;


/* ==================== 全局变量声明 ==================== */

// 声明外部队列句柄，让 main.c 里的主循环能拿到它去“收快递”
extern QueueHandle_t ui_msg_queue;


/* ==================== 函数接口声明 ==================== */

/**
 * @brief  启动 MQTT 客户端并初始化 UI 消息队列
 */
void mqtt_app_start(void);

/**
 * @brief  发送 MQTT 消息 (给 AI 视觉联动等模块专属调用)
 * @param  topic    MQTT 主题
 * @param  payload  要发送的 JSON 或字符串数据
 */
void sensehub_publish_message(const char* topic, const char* payload);

#ifdef __cplusplus
}
#endif

#endif // _MQTT_APP_H_