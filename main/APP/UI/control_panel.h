/**
 ******************************************************************************
 * @file        control_panel.h
 * @version     V3.0
 * @brief       家庭留言便签板 - 头文件
 ******************************************************************************
 * @attention   替代原有的硬件控制面板
 *              温馨的便签留言板界面，适合老年人阅读
 ******************************************************************************
 */

#ifndef __CONTROL_PANEL_H
#define __CONTROL_PANEL_H

#include "lvgl.h"
#include "ai_assistant_ui.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief       创建家庭留言便签板
 * @param       无
 * @retval      屏幕对象指针
 */
lv_obj_t* create_control_panel(void);

/**
 * @brief       删除家庭留言便签板
 * @param       无
 * @retval      无
 */
void delete_control_panel(void);

/**
 * @brief       模拟接收新便签（测试接口）
 * @param       sender: 发送者名称
 * @param       message: 消息内容
 * @param       time: 时间字符串
 * @retval      无
 */
void sticky_note_add_message(const char *sender, const char *message, const char *time);

#ifdef __cplusplus
}
#endif

#endif /* __CONTROL_PANEL_H */
