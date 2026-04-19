/**
 ******************************************************************************
 * @file        lvgl_demo.h
 * @version     V1.0
 * @brief       LVGL V8移植 实验
 ******************************************************************************
 * @attention   Waiken-Smart 慧勤智远
 * 
 * 实验平台:     慧勤智远 ESP32-P4 开发板
 ******************************************************************************
 */
 
#ifndef __LVGL_DEMO_H
#define __LVGL_DEMO_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "touch.h"
#include "lcd.h"
#include "lvgl.h"
#include "esp_timer.h"
#include "demos/lv_demos.h"
#include "ai_assistant_ui.h"


/* 函数声明 */
void lvgl_demo(void);   /* lvgl_demo入口函数 */

#endif
