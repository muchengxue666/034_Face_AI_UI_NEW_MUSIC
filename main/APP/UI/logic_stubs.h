/**
 ******************************************************************************
 * @file        logic_stubs.h
 * @version     V1.0
 * @brief       业务逻辑桩函数 - 头文件
 ******************************************************************************
 * @attention   定义用于前端UI逻辑验证的模拟接口
 *              所有硬件触发都通过这些桩函数模拟
 ******************************************************************************
 */

#ifndef __LOGIC_STUBS_H
#define __LOGIC_STUBS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== LLM 状态枚举 ==================== */
typedef enum {
    LLM_STATE_IDLE,             // 空闲状态
    LLM_STATE_LISTENING,        // 倾听中（录音）
    LLM_STATE_THINKING,         // 思考中（等待LLM响应）
    LLM_STATE_SPEAKING          // 豆包回复播放中（TTS播放）
} LLM_STATE_E;

/* ==================== 模拟接口函数声明 ==================== */

/**
 * @brief       模拟距离感应唤醒触发
 * @param       无
 * @retval      无
 * @attention   未来通过 WIFI CSI 扰动检测或红外传感器触发
 *              目前仅打印日志并触发 UI 唤醒动画
 */
void simulate_proximity_wake_up(void);

/**
 * @brief       模拟 NFC/RC522 刷卡触发
 * @param       card_id: 卡片ID
 * @retval      无
 * @attention   未来通过 RC522 读卡器读取 NFC 卡片
 *              可用于老人身份识别、快捷指令触发等
 */
void simulate_nfc_card_swipe(uint32_t card_id);

/**
 * @brief       更新 LLM 语音对话状态机
 * @param       state: 新的状态
 * @retval      无
 * @attention   控制 UI 显示对应的状态指示
 *              - LISTENING: 显示录音动画
 *              - THINKING: 显示思考动画
 *              - SPEAKING: 显示语音播放动画
 */
void update_llm_state(LLM_STATE_E state);

/**
 * @brief       获取当前 LLM 状态
 * @param       无
 * @retval      当前状态
 */
LLM_STATE_E get_llm_state(void);

/**
 * @brief       模拟手势识别触发
 * @param       gesture: 手势名称（如 "wave_left", "wave_right", "fist"）
 * @retval      无
 * @attention   未来通过摄像头 + AI 模型识别手势
 */
void simulate_gesture_recognition(const char *gesture);

/**
 * @brief       模拟接收家人消息
 * @param       sender: 发送者名称
 * @param       message: 消息内容
 * @retval      无
 * @attention   未来通过 MQTT/WebSocket 从手机 App 接收
 */
void simulate_family_message(const char *sender, const char *message);

#ifdef __cplusplus
}
#endif

#endif /* __LOGIC_STUBS_H */
