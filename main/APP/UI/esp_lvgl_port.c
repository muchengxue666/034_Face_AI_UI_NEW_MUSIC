#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_lvgl_port.h"
#include "esp_lvgl_port_priv.h"
#include "lvgl.h"

static const char *TAG = "LVGL";

#define ESP_LVGL_PORT_TASK_MUX_DELAY_MS    10000

/*******************************************************************************
* 类型定义
*******************************************************************************/

/**
 * @brief LVGL端口上下文结构体
 */
typedef struct lvgl_port_ctx_s {
    TaskHandle_t        lvgl_task;        /*!< LVGL任务句柄 */
    SemaphoreHandle_t   lvgl_mux;         /*!< LVGL互斥锁 */
    SemaphoreHandle_t   task_mux;         /*!< 任务互斥锁 */
    esp_timer_handle_t  tick_timer;       /*!< 定时器句柄 */
    bool                running;          /*!< LVGL任务是否正在运行 */
    int                 task_max_sleep_ms;/*!< LVGL任务最大休眠时间（毫秒） */
    int                 timer_period_ms;  /*!< 定时器周期（毫秒） */
} lvgl_port_ctx_t;

/*******************************************************************************
* 本地变量
*******************************************************************************/
static lvgl_port_ctx_t lvgl_port_ctx; /*!< LVGL端口上下文实例 */

/*******************************************************************************
* 函数定义
*******************************************************************************/
static void lvgl_port_task(void *arg);
static esp_err_t lvgl_port_tick_init(void);
static void lvgl_port_task_deinit(void);

/*******************************************************************************
* 公共API函数
*******************************************************************************/

/**
 * @brief 初始化LVGL端口
 *
 * @param cfg 配置参数
 * @return esp_err_t 错误码
 */
esp_err_t lvgl_port_init(const lvgl_port_cfg_t *cfg)
{
    esp_err_t ret = ESP_OK;
    ESP_GOTO_ON_FALSE(cfg, ESP_ERR_INVALID_ARG, err, TAG, "invalid argument");
    ESP_GOTO_ON_FALSE(cfg->task_affinity < (configNUM_CORES), ESP_ERR_INVALID_ARG, err, TAG, "Bad core number for task! Maximum core number is %d", (configNUM_CORES - 1));

    memset(&lvgl_port_ctx, 0, sizeof(lvgl_port_ctx));

    /* 初始化LVGL */
    lv_init();
    /* 初始化定时器 */
    lvgl_port_ctx.timer_period_ms = cfg->timer_period_ms;
    ESP_RETURN_ON_ERROR(lvgl_port_tick_init(), TAG, "");
    /* 创建任务 */
    lvgl_port_ctx.task_max_sleep_ms = cfg->task_max_sleep_ms;

    if (lvgl_port_ctx.task_max_sleep_ms == 0)
    {
        lvgl_port_ctx.task_max_sleep_ms = 500;
    }

    /* 创建LVGL互斥锁 */
    lvgl_port_ctx.lvgl_mux = xSemaphoreCreateRecursiveMutex();
    ESP_GOTO_ON_FALSE(lvgl_port_ctx.lvgl_mux, ESP_ERR_NO_MEM, err, TAG, "Create LVGL mutex fail!");
    /* 创建任务互斥锁 */
    lvgl_port_ctx.task_mux = xSemaphoreCreateMutex();
    ESP_GOTO_ON_FALSE(lvgl_port_ctx.task_mux, ESP_ERR_NO_MEM, err, TAG, "Create LVGL task sem fail!");

    BaseType_t res;

    if (cfg->task_affinity < 0)
    {
        res = xTaskCreate(lvgl_port_task, "taskLVGL", cfg->task_stack, NULL, cfg->task_priority, &lvgl_port_ctx.lvgl_task);
    }
    else
    {
        res = xTaskCreatePinnedToCore(lvgl_port_task, "taskLVGL", cfg->task_stack, NULL, cfg->task_priority, &lvgl_port_ctx.lvgl_task, cfg->task_affinity);
    }

    ESP_GOTO_ON_FALSE(res == pdPASS, ESP_FAIL, err, TAG, "Create LVGL task fail!");

err:

    if (ret != ESP_OK)
    {
        lvgl_port_deinit();
    }

    return ret;
}

/**
 * @brief 恢复LVGL端口任务
 *
 * @return esp_err_t 错误码
 */
esp_err_t lvgl_port_resume(void)
{
    esp_err_t ret = ESP_ERR_INVALID_STATE;

    if (lvgl_port_ctx.tick_timer != NULL)
    {
        lv_timer_enable(true);
        ret = esp_timer_start_periodic(lvgl_port_ctx.tick_timer, lvgl_port_ctx.timer_period_ms * 1000);
    }

    return ret;
}

/**
 * @brief 停止LVGL端口任务
 *
 * @return esp_err_t 错误码
 */
esp_err_t lvgl_port_stop(void)
{
    esp_err_t ret = ESP_ERR_INVALID_STATE;

    if (lvgl_port_ctx.tick_timer != NULL)
    {
        lv_timer_enable(false);
        ret = esp_timer_stop(lvgl_port_ctx.tick_timer);
    }

    return ret;
}

/**
 * @brief 反初始化LVGL端口
 *
 * @return esp_err_t 错误码
 */
esp_err_t lvgl_port_deinit(void)
{
    /* 停止并删除定时器 */
    if (lvgl_port_ctx.tick_timer != NULL)
    {
        esp_timer_stop(lvgl_port_ctx.tick_timer);
        esp_timer_delete(lvgl_port_ctx.tick_timer);
        lvgl_port_ctx.tick_timer = NULL;
    }

    /* 停止运行任务 */
    if (lvgl_port_ctx.running)
    {
        lvgl_port_ctx.running = false;
    }

    /* 等待任务停止 */
    if (xSemaphoreTake(lvgl_port_ctx.task_mux, pdMS_TO_TICKS(ESP_LVGL_PORT_TASK_MUX_DELAY_MS)) != pdTRUE)
    {
        ESP_LOGE(TAG, "Failed to stop LVGL task");
        return ESP_ERR_TIMEOUT;
    }
    ESP_LOGI(TAG, "Stopped LVGL task");

    lvgl_port_task_deinit();

    return ESP_OK;
}

/**
 * @brief 获取LVGL互斥锁
 *
 * @param timeout_ms 超时时间，单位为毫秒。0表示无限期阻塞。
 * @return bool 是否成功获取互斥锁
 */
bool lvgl_port_lock(uint32_t timeout_ms)
{
    assert(lvgl_port_ctx.lvgl_mux && "lvgl_port_init must be called first");

    const TickType_t timeout_ticks = (timeout_ms == 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(lvgl_port_ctx.lvgl_mux, timeout_ticks) == pdTRUE;
}

/**
 * @brief 释放LVGL互斥锁
 */
void lvgl_port_unlock(void)
{
    assert(lvgl_port_ctx.lvgl_mux && "lvgl_port_init must be called first");
    xSemaphoreGiveRecursive(lvgl_port_ctx.lvgl_mux);
}

/**
 * @brief 唤醒LVGL任务
 *
 * @param event 事件类型
 * @param param 用户参数
 * @return esp_err_t 错误码
 */
esp_err_t lvgl_port_task_wake(lvgl_port_event_type_t event, void *param)
{
    ESP_LOGE(TAG, "Task wake is not supported, when used LVGL8!");
    return ESP_ERR_NOT_SUPPORTED;
}

/**
 * @brief 通知LVGL任务
 *
 * @param value 通知的值
 * @return bool 是否需要任务切换
 */
IRAM_ATTR bool lvgl_port_task_notify(uint32_t value)
{
    BaseType_t need_yield = pdFALSE;

    /* 通知LVGL任务 */
    if (xPortInIsrContext() == pdTRUE)
    {
        xTaskNotifyFromISR(lvgl_port_ctx.lvgl_task, value, eNoAction, &need_yield);
    }
    else
    {
        xTaskNotify(lvgl_port_ctx.lvgl_task, value, eNoAction);
    }

    return (need_yield == pdTRUE);
}

/*******************************************************************************
* 私有函数
*******************************************************************************/

/**
 * @brief LVGL任务函数
 *
 * @param arg 参数
 */
static void lvgl_port_task(void *arg)
{
    uint32_t task_delay_ms = lvgl_port_ctx.task_max_sleep_ms;

    /* 获取任务互斥锁 */
    if (xSemaphoreTake(lvgl_port_ctx.task_mux, 0) != pdTRUE)
    {
        ESP_LOGE(TAG, "Failed to take LVGL task sem");
        lvgl_port_task_deinit();
        vTaskDelete( NULL );
    }

    ESP_LOGI(TAG, "Starting LVGL task");
    lvgl_port_ctx.running = true;

    while (lvgl_port_ctx.running)
    {
        if (lvgl_port_lock(0))
        {
            task_delay_ms = lv_timer_handler();
            lvgl_port_unlock();
        }

        if (task_delay_ms > lvgl_port_ctx.task_max_sleep_ms)
        {
            task_delay_ms = lvgl_port_ctx.task_max_sleep_ms;
        }
        else if (task_delay_ms < 5)
        {
            task_delay_ms = 5;
        }
        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }

    /* 释放任务互斥锁 */
    xSemaphoreGive(lvgl_port_ctx.task_mux);

    /* 关闭任务 */
    vTaskDelete( NULL );
}

/**
 * @brief 反初始化LVGL任务
 */
static void lvgl_port_task_deinit(void)
{
    if (lvgl_port_ctx.lvgl_mux)
    {
        vSemaphoreDelete(lvgl_port_ctx.lvgl_mux);
    }

    if (lvgl_port_ctx.task_mux)
    {
        vSemaphoreDelete(lvgl_port_ctx.task_mux);
    }

    memset(&lvgl_port_ctx, 0, sizeof(lvgl_port_ctx));

#if LV_ENABLE_GC || !LV_MEM_CUSTOM
    /* 反初始化LVGL */
    lv_deinit();
#endif
}

/**
 * @brief 增加LVGL定时器
 *
 * @param arg 参数
 */
static void lvgl_port_tick_increment(void *arg)
{
    /* 告诉LVGL已过去多少毫秒 */
    lv_tick_inc(lvgl_port_ctx.timer_period_ms);
}

/**
 * @brief 初始化LVGL定时器
 *
 * @return esp_err_t 错误码
 */
static esp_err_t lvgl_port_tick_init(void)
{
    /* 使用esp_timer生成2ms周期事件作为LVGL的定时器 */
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &lvgl_port_tick_increment,
        .name = "LVGL tick",
    };
    ESP_RETURN_ON_ERROR(esp_timer_create(&lvgl_tick_timer_args, &lvgl_port_ctx.tick_timer), TAG, "Failed to take LVGL task timer");
    return esp_timer_start_periodic(lvgl_port_ctx.tick_timer, lvgl_port_ctx.timer_period_ms * 1000);
}
