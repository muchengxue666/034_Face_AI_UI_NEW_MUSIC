/**
 * @file    gesture_ai.h
 * @brief   ESP-DL 手势识别封装（C 接口）
 * @note    支持手势：one/two/three/four/five/like/ok/call/dislike
 */
#ifndef GESTURE_AI_H
#define GESTURE_AI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 手势类型（与 ESP-DL hand_gesture_category_name 对应）*/
typedef enum {
    GESTURE_NONE = 0,       /* 无手/无手势 */
    GESTURE_ONE,            /* 伸食指（1）*/
    GESTURE_TWO,            /* 剪刀手（2）*/
    GESTURE_THREE,          /* 伸三指（3）*/
    GESTURE_FOUR,           /* 伸四指（4）*/
    GESTURE_FIVE,           /* 张开手掌（5）*/
    GESTURE_LIKE,           /* 竖大拇指（赞）*/
    GESTURE_OK,             /* OK 手势 */
    GESTURE_CALL,           /* 打电话手势 */
    GESTURE_DISLIKE,        /* 大拇指朝下（踩）*/
} gesture_type_t;

/* 手势识别结果 */
typedef struct {
    gesture_type_t type;
    const char *name;       /* 原始类别名称字符串 */
    float confidence;
    int box_x1, box_y1, box_x2, box_y2;  /* 手的位置框 */
} gesture_result_t;

void init_gesture_ai(void);
gesture_type_t process_gesture_frame(uint8_t* pixels, int width, int height, gesture_result_t *result);
const char* gesture_type_to_str(gesture_type_t type);

#ifdef __cplusplus
}
#endif

#endif
