/**
 ******************************************************************************
 * @file        voice_pipeline.h
 * @version     V1.0
 * @brief       语音交互流水线
 *              唤醒词触发 → 录音(STT) → 豆包大模型 → TTS播放
 ******************************************************************************
 */

#ifndef __VOICE_PIPELINE_H
#define __VOICE_PIPELINE_H

#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief 语音流水线状态
 */
typedef enum {
    VOICE_STATE_IDLE = 0,   /* 待机，等待唤醒 */
    VOICE_STATE_LISTENING,  /* 录音中（STT采集）*/
    VOICE_STATE_PROCESSING, /* 调用豆包LLM处理中 */
    VOICE_STATE_SPEAKING,   /* TTS播放中 */
    VOICE_STATE_ERROR,      /* 错误状态 */
} voice_state_t;

/**
 * @brief 状态变化回调（用于通知UI更新）
 */
typedef void (*voice_state_cb_t)(voice_state_t state);

/**
 * @brief 初始化语音流水线
 *        内部会初始化 I2S + ES8311，并启动监听任务
 * @return ESP_OK 成功
 */
esp_err_t voice_pipeline_init(void);

/**
 * @brief 注册状态变化回调
 * @param cb 回调函数
 */
void voice_pipeline_set_state_cb(voice_state_cb_t cb);

/**
 * @brief 软件触发唤醒（模拟唤醒词，供UI按钮调用）
 */
void voice_pipeline_trigger(void);

/**
 * @brief 获取当前状态
 */
voice_state_t voice_pipeline_get_state(void);
esp_err_t voice_pipeline_play_notification_chime(void);
esp_err_t voice_pipeline_speak_text(const char *text);

#endif /* __VOICE_PIPELINE_H */
