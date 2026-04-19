#pragma once

#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "lvgl.h"

#if LVGL_VERSION_MAJOR == 8
#include "esp_lvgl_port_compatibility.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 显示旋转配置结构体
 * 
 * 用于配置显示设备的旋转设置（例如交换X轴和Y轴、翻转X轴或Y轴等）
 */
typedef struct {
    bool swap_xy;  /* 如果为真，X轴和Y轴交换 (在esp_lcd驱动中生效) */
    bool mirror_x; /* 如果为真，X轴镜像翻转 (在esp_lcd驱动中生效) */
    bool mirror_y; /* 如果为真，Y轴镜像翻转 (在esp_lcd驱动中生效) */
} lvgl_port_rotation_cfg_t;

/**
 * @brief 显示设备的配置结构体
 * 
 * 用于配置显示面板的各项参数，如IO句柄、分辨率、旋转设置等
 */
typedef struct {
    esp_lcd_panel_io_handle_t io_handle;    /* LCD面板IO句柄 */
    esp_lcd_panel_handle_t panel_handle;    /* LCD面板句柄 */
    esp_lcd_panel_handle_t control_handle;  /* LCD面板控制句柄 */
    uint32_t    buffer_size;                /* 显示屏缓冲区的大小（像素数） */
    bool        double_buffer;              /* 如果为真，分配两个缓冲区 */
    uint32_t    trans_size;                 /* 可选项：分配缓冲区到SRAM以进行帧缓冲区移动 */
    uint32_t    hres;                       /* LCD显示屏的水平分辨率 */
    uint32_t    vres;                       /* LCD显示屏的垂直分辨率 */
    bool        monochrome;                 /* 如果为真，显示器为单色，并使用1位表示1像素 */
    lvgl_port_rotation_cfg_t rotation;      /* 屏幕的默认旋转设置（仅支持硬件旋转，软件旋转不支持） */
#if LVGL_VERSION_MAJOR >= 9
    lv_color_format_t        color_format;  /* 显示屏的颜色格式 */
#endif
    struct {
        unsigned int buff_dma: 1;           /* 如果为真，分配的LVGL缓冲区支持DMA */
        unsigned int buff_spiram: 1;        /* 如果为真，分配的LVGL缓冲区在PSRAM中 */
        unsigned int sw_rotate: 1;          /* 如果为真，使用软件旋转（较慢），如果可用，使用PPA硬件加速 */
#if LVGL_VERSION_MAJOR >= 9
        unsigned int swap_bytes: 1;         /* 如果为真，在RGB656（16位）颜色格式中交换字节，再发送给LCD驱动 */
#endif
        unsigned int full_refresh: 1;       /* 如果为真，始终重新绘制整个屏幕 */
        unsigned int direct_mode: 1;        /* 如果为真，使用屏幕大小的缓冲区并绘制到绝对坐标 */
    } flags;
} lvgl_port_display_cfg_t;

/**
 * @brief 配置RGB显示设备结构体
 * 
 * 用于配置RGB显示设备的特定设置（例如使用bounce buffer模式、防止撕裂等）
 */
typedef struct {
    struct {
        unsigned int bb_mode: 1;        /* 如果为真，使用bounce buffer模式 */
        unsigned int avoid_tearing: 1;  /* 如果为真，使用内部RGB缓冲区作为LVGL绘制缓冲区以避免撕裂效果 */
    } flags;
} lvgl_port_display_rgb_cfg_t;

/**
 * @brief 配置MIPI-DSI显示设备结构体
 * 
 * 用于配置MIPI-DSI显示设备的特定设置（例如防止撕裂）
 */
typedef struct {
    struct {
        unsigned int avoid_tearing: 1;  /* 如果为真，使用内部MIPI-DSI缓冲区作为LVGL绘制缓冲区以避免撕裂效果 */
    } flags;
} lvgl_port_display_dsi_cfg_t;

/**
 * @brief 将I2C/SPI/I8080显示设备添加到LVGL
 *
 * @note 在此函数中分配的内存不会在反初始化时释放。你必须调用lvgl_port_remove_disp来释放所有内存。
 *
 * @param disp_cfg 显示设备配置结构体
 * @return 返回指向LVGL显示设备的指针，如果发生错误则返回NULL
 */
lv_display_t *lvgl_port_add_disp(const lvgl_port_display_cfg_t *disp_cfg);

/**
 * @brief 将MIPI-DSI显示设备添加到LVGL
 *
 * @note 在此函数中分配的内存不会在反初始化时释放。你必须调用lvgl_port_remove_disp来释放所有内存。
 *
 * @param disp_cfg 显示设备配置结构体
 * @param dsi_cfg MIPI-DSI显示设备特定配置结构体
 * @return 返回指向LVGL显示设备的指针，如果发生错误则返回NULL
 */
lv_display_t *lvgl_port_add_disp_dsi(const lvgl_port_display_cfg_t *disp_cfg, const lvgl_port_display_dsi_cfg_t *dsi_cfg);

/**
 * @brief 将RGB显示设备添加到LVGL
 *
 * @note 在此函数中分配的内存不会在反初始化时释放。你必须调用lvgl_port_remove_disp来释放所有内存。
 *
 * @param disp_cfg 显示设备配置结构体
 * @param rgb_cfg RGB显示设备特定配置结构体
 * @return 返回指向LVGL显示设备的指针，如果发生错误则返回NULL
 */
lv_display_t *lvgl_port_add_disp_rgb(const lvgl_port_display_cfg_t *disp_cfg, const lvgl_port_display_rgb_cfg_t *rgb_cfg);

/**
 * @brief 从LVGL中移除显示设备
 *
 * @note 释放此显示设备所占用的所有内存。
 *
 * @return
 *      - ESP_OK                    成功时返回
 */
esp_err_t lvgl_port_remove_disp(lv_display_t *disp);

#ifdef __cplusplus
}
#endif
