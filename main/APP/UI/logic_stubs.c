/**
 ******************************************************************************
 * @file        logic_stubs.c
 * @version     V1.0
 * @brief       业务逻辑桩函数 - 实现文件
 ******************************************************************************
 * @attention   所有函数目前只打印日志并触发对应的 UI 变化
 *              未来接入真实硬件时替换内部实现
 ******************************************************************************
 */

#include "logic_stubs.h"
#include "ai_assistant_ui.h"
#include "control_panel.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "logic_stubs";

/* 当前 LLM 状态 */
static LLM_STATE_E s_current_llm_state = LLM_STATE_IDLE;

/**
 * @brief       获取 LLM 状态名称字符串
 * @param       state: 状态枚举值
 * @retval      状态名称字符串
 */
static const char* llm_state_to_str(LLM_STATE_E state)
{
    switch (state) {
        case LLM_STATE_IDLE:      return "IDLE";
        case LLM_STATE_LISTENING: return "LISTENING";
        case LLM_STATE_THINKING:  return "THINKING";
        case LLM_STATE_SPEAKING:  return "SPEAKING";
        default:                  return "UNKNOWN";
    }
}

/**
 * @brief       模拟距离感应唤醒触发
 */
void simulate_proximity_wake_up(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "【桩函数】距离感应唤醒触发");
    ESP_LOGI(TAG, "【说明】未来将通过 WIFI CSI 扰动检测实现");
    ESP_LOGI(TAG, "【动作】触发 UI 唤醒动画");
    ESP_LOGI(TAG, "========================================");

    /* 触发 UI 唤醒 */
    ui_trigger_wake_up();
}

/**
 * @brief       模拟 NFC/RC522 刷卡触发
 */
void simulate_nfc_card_swipe(uint32_t card_id)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "【桩函数】NFC 刷卡触发");
    ESP_LOGI(TAG, "【卡片ID】0x%08lX", (unsigned long)card_id);
    ESP_LOGI(TAG, "【说明】未来将通过 RC522 读卡器读取");
    ESP_LOGI(TAG, "========================================");

    /* 根据卡片 ID 执行不同操作 */
    if (card_id == 0x12345678) {
        ESP_LOGI(TAG, "【识别】主用户卡 -> 显示个性化问候");
        /* 未来可以根据卡片显示个性化界面 */
    } else if (card_id == 0xAABBCCDD) {
        ESP_LOGI(TAG, "【识别】访客卡 -> 切换到访客模式");
    } else {
        ESP_LOGI(TAG, "【识别】未知卡片 -> 显示通用界面");
    }

    /* 触发 UI 反馈 */
    ui_show_gesture_feedback("已识别卡片");
}

/**
 * @brief       更新 LLM 语音对话状态机
 */
void update_llm_state(LLM_STATE_E state)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "【桩函数】LLM 状态机更新");
    ESP_LOGI(TAG, "【旧状态】%s", llm_state_to_str(s_current_llm_state));
    ESP_LOGI(TAG, "【新状态】%s", llm_state_to_str(state));
    ESP_LOGI(TAG, "========================================");

    s_current_llm_state = state;

    /* 根据状态触发对应的 UI 更新 */
    switch (state) {
        case LLM_STATE_IDLE:
            ESP_LOGI(TAG, "【UI动作】隐藏状态指示器");
            break;

        case LLM_STATE_LISTENING:
            ESP_LOGI(TAG, "【UI动作】显示录音动画 + 麦克风图标");
            ui_show_gesture_feedback("正在倾听...");
            break;

        case LLM_STATE_THINKING:
            ESP_LOGI(TAG, "【UI动作】显示思考动画 + 等待提示");
            ui_show_gesture_feedback("思考中...");
            break;

        case LLM_STATE_SPEAKING:
            ESP_LOGI(TAG, "【UI动作】显示语音播放动画 + 波形");
            ui_show_gesture_feedback("豆包回复中...");
            break;

        default:
            break;
    }
}

/**
 * @brief       获取当前 LLM 状态
 */
LLM_STATE_E get_llm_state(void)
{
    return s_current_llm_state;
}

/**
 * @brief       模拟手势识别触发
 */
void simulate_gesture_recognition(const char *gesture)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "【桩函数】手势识别触发");
    ESP_LOGI(TAG, "【手势】%s", gesture);
    ESP_LOGI(TAG, "【说明】未来将通过摄像头 + AI 模型识别");
    ESP_LOGI(TAG, "========================================");

    /* 根据手势执行不同操作 */
    if (strcmp(gesture, "wave_left") == 0) {
        ESP_LOGI(TAG, "【动作】切换到上一首音乐");
        ui_show_gesture_feedback("◀ 上一首");
    } else if (strcmp(gesture, "wave_right") == 0) {
        ESP_LOGI(TAG, "【动作】切换到下一首音乐");
        ui_show_gesture_feedback("下一首 ▶");
    } else if (strcmp(gesture, "fist") == 0) {
        ESP_LOGI(TAG, "【动作】暂停/播放");
        ui_show_gesture_feedback("⏸ 暂停");
    } else if (strcmp(gesture, "palm") == 0) {
        ESP_LOGI(TAG, "【动作】停止播放");
        ui_show_gesture_feedback("⏹ 停止");
    } else if (strcmp(gesture, "thumbs_up") == 0) {
        ESP_LOGI(TAG, "【动作】点赞收藏");
        ui_show_gesture_feedback("👍 已收藏");
    } else {
        ESP_LOGI(TAG, "【动作】未知手势，忽略");
    }
}

/**
 * @brief       模拟接收家人消息
 */
void simulate_family_message(const char *sender, const char *message)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "【桩函数】接收家人消息");
    ESP_LOGI(TAG, "【发送者】%s", sender);
    ESP_LOGI(TAG, "【消息】%s", message);
    ESP_LOGI(TAG, "【说明】未来将通过 MQTT/WebSocket 接收");
    ESP_LOGI(TAG, "========================================");

    /* 添加到便签板 */
    sticky_note_add_message(sender, message, "刚刚");

    /* 显示通知 */
    char notice[64];
    snprintf(notice, sizeof(notice), "新消息：来自%s", sender);
    ui_show_gesture_feedback(notice);
}
