#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/errno.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "linux/videodev2.h"
#include "esp_video_init.h"
#include "app_video.h"

static const char *TAG = "app_video";

#define MAX_BUFFER_COUNT                (3)
uint8_t *buffer[MAX_BUFFER_COUNT];
uint32_t buffer_size[MAX_BUFFER_COUNT];

esp_err_t app_video_init(int fd, video_fmt_t init_fmt)
{
    esp_err_t ret = ESP_OK;
    int fmt_index = 0;
    struct v4l2_format init_format;
    struct v4l2_capability capability;
    const int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(fd, VIDIOC_QUERYCAP, &capability)) {
        ESP_LOGE(TAG, "failed to get capability");
        ret = ESP_FAIL;
        goto exit_0;
    }

    ESP_LOGI(TAG, "version: %d.%d.%d", (uint16_t)(capability.version >> 16),
             (uint8_t)(capability.version >> 8),
             (uint8_t)capability.version);
    ESP_LOGI(TAG, "driver:  %s", capability.driver);
    ESP_LOGI(TAG, "card:    %s", capability.card);
    ESP_LOGI(TAG, "bus:     %s", capability.bus_info);

    memset(&init_format, 0, sizeof(struct v4l2_format));
    init_format.type = type;
    if (ioctl(fd, VIDIOC_G_FMT, &init_format) != 0) {
        ESP_LOGE(TAG, "failed to get format");
        ret = ESP_FAIL;
        goto exit_0;
    }

    ESP_LOGI(TAG, "width=%" PRIu32 " height=%" PRIu32, init_format.fmt.pix.width, init_format.fmt.pix.height);

    while (1) {
        struct v4l2_fmtdesc fmtdesc = {
            .index = fmt_index++,
            .type = type,
        };

        if (ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) != 0) {
            ESP_LOGW(TAG, "enum all fmt");
            ret = ESP_ERR_INVALID_ARG;
            goto exit_0;
        }

        if (fmtdesc.pixelformat != init_fmt) {
            continue;
        }

        struct v4l2_format format = {
            .type = type,
            .fmt.pix.width = init_format.fmt.pix.width,
            .fmt.pix.height = init_format.fmt.pix.height,
            .fmt.pix.pixelformat = fmtdesc.pixelformat,
        };

        if (ioctl(fd, VIDIOC_S_FMT, &format) != 0) {
            if (errno == ESRCH) {
                continue;
            } else {
                ESP_LOGE(TAG, "failed to set format");
                ret = ESP_FAIL;
                goto exit_0;
            }
        }
        ESP_LOGI(TAG, "Capture %s format", (char *)fmtdesc.description);
        break;
    }
    return ret;

exit_0:
    close(fd);
    return ret;
}

static int video_open(int port)
{
    int ret;
    char name[16];
    int fd = -1;

    ret = snprintf(name, sizeof(name), "/dev/video%d", port);
    if (ret <= 0) {
        return ESP_FAIL;
    }

    fd = open(name, O_RDONLY);
    if (fd < 0) {
        ESP_LOGE(TAG, "Open video %s fail", name);
        return -1;
    }

    return fd;
}

int app_video_open(int port)
{
    int fd = video_open(port);
    if (fd < 0) {
        ESP_LOGE(TAG, "Open video fail");
    }
    return fd;
}

esp_err_t camera_stream_start(int video_fd)
{
    ESP_LOGI(TAG, "Camera Start");

    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(video_fd, VIDIOC_STREAMON, &type)) {
        ESP_LOGE(TAG, "failed to start stream");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t camera_stream_stop(int video_fd)
{
    ESP_LOGI(TAG, "Camera Stop");

    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(video_fd, VIDIOC_STREAMOFF, &type)) {
        ESP_LOGE(TAG, "failed to stop stream");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t camera_set_bufs(int video_fd, int fb_num, void **fb) 
{
    if (fb_num > MAX_BUFFER_COUNT) {
        ESP_LOGE(TAG, "buffer num is too large");
        return ESP_FAIL;
    }

    struct v4l2_requestbuffers req;
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    memset(&req, 0, sizeof(req));
    req.count = fb_num;
    req.type = type;

    req.memory = fb ? V4L2_MEMORY_USERPTR : V4L2_MEMORY_MMAP;

    if (ioctl(video_fd, VIDIOC_REQBUFS, &req) != 0) {
        ESP_LOGE(TAG, "req bufs failed");
        goto errout_req_bufs;
    }

    for (int i = 0; i < fb_num; i++) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = type;
        buf.memory = req.memory;
        buf.index = i;

        if (ioctl(video_fd, VIDIOC_QUERYBUF, &buf) != 0) {
            ESP_LOGE(TAG, "query buf failed");
            goto errout_req_bufs;
        }

        if (req.memory == V4L2_MEMORY_MMAP) {
            buffer[i] = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, video_fd, buf.m.offset);
            if (buffer[i] == NULL) {
                ESP_LOGE(TAG, "mmap failed");
                goto errout_req_bufs;
            }
            buffer_size[i] = buf.length;
        } else {
            if (!fb[i]) {
                ESP_LOGE(TAG, "frame buffer is NULL");
                goto errout_req_bufs;
            }
            buf.m.userptr = (unsigned long)fb[i];
            buffer[i] = fb[i];
            buffer_size[i] = buf.length;
        }

        if (ioctl(video_fd, VIDIOC_QBUF, &buf) != 0) {
            ESP_LOGE(TAG, "queue frame buffer failed");
            goto errout_req_bufs;
        }
    }

    return ESP_OK;

errout_req_bufs:
    close(video_fd);
    return ESP_FAIL;
}

esp_err_t camera_get_bufs(int fb_num, void **fb)
{
    if(fb_num > MAX_BUFFER_COUNT) {
        ESP_LOGE(TAG, "buffer num is too large");
        return ESP_FAIL;
    }

    for(int i = 0; i < fb_num; i++) {
        if(buffer[i] != NULL) {
            fb[i] = buffer[i];
        } else {
            ESP_LOGE(TAG, "frame buffer is NULL");
            return ESP_FAIL;
        }
    }

    return ESP_OK;
}

uint32_t camera_get_buf_size(int fb_count)
{
    if(fb_count > MAX_BUFFER_COUNT) {
        ESP_LOGE(TAG, "buffer num is too large");
        return 0;
    }

    return buffer_size[fb_count];
}

// 核心修改：砍掉宏定义束缚的强制初始化版！
esp_err_t app_video_main(i2c_master_bus_handle_t i2c_bus_handle)
{
    // 🪄 魔法 1：只声明为 static（强行分配到内部 RAM），先不给初始值！
    static esp_video_init_csi_config_t csi_config[1];
    static esp_video_init_config_t cam_config;

    // 🪄 魔法 2：在函数内部动态赋值！完美规避 C 编译器的“死板语法检查”！
    csi_config[0].sccb_config.init_sccb = true;
    csi_config[0].sccb_config.i2c_config.port = 0;
    csi_config[0].sccb_config.i2c_config.scl_pin = -1;
    csi_config[0].sccb_config.i2c_config.sda_pin = -1;
    csi_config[0].sccb_config.freq = 400000;
    csi_config[0].reset_pin = -1;
    csi_config[0].pwdn_pin = -1;

    if(i2c_bus_handle != NULL)
    {
        csi_config[0].sccb_config.init_sccb = false;
        csi_config[0].sccb_config.i2c_handle = i2c_bus_handle;
    }

    // 把装满参数的内部 RAM 数组，挂载到同样在内部 RAM 的 cam_config 上
    cam_config.csi = csi_config;

    // 完美交接给硬件！
    return esp_video_init(&cam_config);
}