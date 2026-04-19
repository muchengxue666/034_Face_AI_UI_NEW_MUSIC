/**
 ******************************************************************************
 * @file        viewer_screen.h
 * @version     V1.0
 * @brief       知识库与影像视图 - 头文件
 ******************************************************************************
 * @attention   支持手势控制的影像查看和知识库文档显示
 ******************************************************************************
 */

#ifndef __VIEWER_SCREEN_H
#define __VIEWER_SCREEN_H

#include "lvgl.h"
#include "ai_assistant_ui.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 函数声明 */
lv_obj_t* create_viewer_screen(void);
void delete_viewer_screen(void);

#ifdef __cplusplus
}
#endif

#endif /* __VIEWER_SCREEN_H */