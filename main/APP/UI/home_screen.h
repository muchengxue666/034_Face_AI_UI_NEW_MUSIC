/**
 ******************************************************************************
 * @file        home_screen.h
 * @version     V1.0
 * @brief       多模态交互主页 - 头文件
 ******************************************************************************
 * @attention   包含视觉区、对话区和音频波形动画
 ******************************************************************************
 */

#ifndef __HOME_SCREEN_H
#define __HOME_SCREEN_H

#include "lvgl.h"
#include "ai_assistant_ui.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 函数声明 */
lv_obj_t* create_home_screen(void);
void delete_home_screen(void);

#ifdef __cplusplus
}
#endif

#endif /* __HOME_SCREEN_H */