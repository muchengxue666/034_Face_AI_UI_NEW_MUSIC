#ifndef __APP_VIDEO_H
#define __APP_VIDEO_H

#include "esp_err.h"
#include "linux/videodev2.h"

#include "driver/i2c_master.h"

typedef enum {
    APP_VIDEO_FMT_RAW8 = V4L2_PIX_FMT_SBGGR8,
    APP_VIDEO_FMT_RAW10 = V4L2_PIX_FMT_SBGGR10,
    APP_VIDEO_FMT_GREY = V4L2_PIX_FMT_GREY,
    APP_VIDEO_FMT_RGB565 = V4L2_PIX_FMT_RGB565,
    APP_VIDEO_FMT_RGB888 = V4L2_PIX_FMT_RGB24,
    APP_VIDEO_FMT_YUV422 = V4L2_PIX_FMT_YUV422P,
    APP_VIDEO_FMT_YUV420 = V4L2_PIX_FMT_YUV420,
} video_fmt_t;


/**
 * @brief Initialize the video application.
 *
 * This function initializes the video application with the given file descriptor
 * and format.
 *
 * @param fd File descriptor for the video device
 * @param init_fmt Initial format for the video device
 * @return 
 *    - ESP_OK: Success
 *    - ESP_FAIL: Failure
 */
esp_err_t app_video_init(int fd, video_fmt_t init_fmt);

/**
 * @brief Main function for video application.
 *
 * This function initializes and starts the video application.
 *
 * @return 
 *    - ESP_OK: Success
 *    - ESP_FAIL: Failure
 */
esp_err_t app_video_main(i2c_master_bus_handle_t i2c_bus_handle);

/**
 * @brief Open the video device.
 *
 * This function opens the video device on the specified port.
 *
 * @param port Port number for the video device
 * @return 
 *    - File descriptor of the opened video device on success
 *    - -1 on failure
 */
int app_video_open(int port);

/**
 * @brief Start the camera stream.
 *
 * This function starts the camera stream using the given file descriptor.
 *
 * @param video_fd File descriptor for the video device
 * @return 
 *    - ESP_OK: Success
 *    - ESP_FAIL: Failure
 */
esp_err_t camera_stream_start(int video_fd);

/**
 * @brief Stop the camera stream.
 *
 * This function stops the camera stream using the given file descriptor.
 *
 * @param video_fd File descriptor for the video device
 * @return 
 *    - ESP_OK: Success
 *    - ESP_FAIL: Failure
 */
esp_err_t camera_stream_stop(int video_fd);

/**
 * @brief Request buffers for the camera stream.
 *
 * This function requests buffers for the camera stream using the given file descriptor.
 *
 * @param video_fd File descriptor for the video device
 * @param fb_num Number of frame buffers to request
 * @param fb Array of frame buffers
 * @return 
 *    - ESP_OK: Success
 *    - ESP_FAIL: Failure
 */
esp_err_t camera_set_bufs(int video_fd, int fb_num, void **fb);

/**
 * @brief Get buffers for the camera stream.
 *
 * This function gets buffers for the camera stream using the given file descriptor.
 *
 * @param fb_num Number of frame buffers to get
 * @param fb Array of frame buffers
 * @return 
 *    - ESP_OK: Success
 *    - ESP_FAIL: Failure
 */
esp_err_t camera_get_bufs(int fb_num, void **fb);

/**
 * @brief Get buffer size for the camera stream.
 *
 * This function gets the buffer size for the camera stream using the given file descriptor.
 *
 * @param fb_num Number of frame buffers to get
 * @param size Pointer to the buffer size
 * @return 
 *    - ESP_OK: Success
 *    - ESP_FAIL: Failure
 */
uint32_t camera_get_buf_size(int fb_num);


#endif