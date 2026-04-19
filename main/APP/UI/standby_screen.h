/**
 ******************************************************************************
 * @file        standby_screen.h
 * @version     V1.0
 * @brief       息屏唤醒页 - 头文件
 ******************************************************************************
 * @attention   实现全黑背景下的呼吸光晕效果和唤醒转场动画
 ******************************************************************************
 */

#ifndef __STANDBY_SCREEN_H
#define __STANDBY_SCREEN_H

#include "lvgl.h"
#include "ai_assistant_ui.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 函数声明 */
lv_obj_t* create_standby_screen(void);
void delete_standby_screen(void);
void trigger_standby_wake_up(void);
void restart_breathing_animations(void);

#ifdef __cplusplus
}
#endif

#endif /* __STANDBY_SCREEN_H */