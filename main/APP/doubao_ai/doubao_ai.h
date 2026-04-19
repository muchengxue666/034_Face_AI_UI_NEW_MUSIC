#ifndef __DOUBAO_AI_H
#define __DOUBAO_AI_H

/**
 * @brief 呼叫豆包大模型处理留言（异步执行，不卡主线程）
 * @param family_message 家属的留言内容
 */
void start_doubao_chat(const char* family_message);

#endif