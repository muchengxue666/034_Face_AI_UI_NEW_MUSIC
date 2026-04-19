#ifndef FACE_AI_WRAPPER_H
#define FACE_AI_WRAPPER_H

#include <stdint.h>

/* 这句是魔法：告诉 C++ 编译器，下面的函数要按照 C 语言的规则来打包，
   这样你的 mipi_cam.c 才能认识它们！ */
#ifdef __cplusplus
extern "C" {
#endif

// 唤醒 AI 大脑
void init_face_ai(void);

// 把摄像头的画面喂给 AI，让它找人脸
void process_face_frame(uint8_t* pixels, int width, int height);

#ifdef __cplusplus
}
#endif

#endif