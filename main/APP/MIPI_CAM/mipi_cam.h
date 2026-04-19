/**
 ******************************************************************************
 * @file        mipi_cam.h
 * @version     V1.0
 * @brief       mipicamera驱动代码
 ******************************************************************************
 * @attention   Waiken-Smart 慧勤智远
 * 
 * 实验平台:     慧勤智远 ESP32-P4 开发板
 ******************************************************************************
 */

#ifndef __MIPI_CAM_H
#define __MIPI_CAM_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "app_video.h"
#include "myiic.h"
#include "driver/ppa.h"
#include "esp_timer.h"
#include "lcd.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/errno.h>
#include "esp_log.h"
#include "esp_private/esp_cache_private.h"
#include <stdbool.h>


/* 函数声明 */
esp_err_t mipi_cam_init(void);          /* mipi_cam初始化 */
esp_err_t mipi_cam_set_paused(bool paused);
bool      mipi_cam_is_paused(void);

#endif
