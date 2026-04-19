/**
 * @file    gesture_ai.cpp
 * @brief   ESP-DL 手势识别封装
 * @note    两步：HandDetect 检测手 → HandGestureRecognizer 识别手势
 *          支持: one/two/three/four/five/like/ok/call/dislike
 */
#include "gesture_ai.h"
#include "hand_detect.hpp"
#include "hand_gesture_recognition.hpp"
#include "dl_image_define.hpp"
#include "esp_log.h"
#include <algorithm>
#include <string.h>

static const char *TAG = "GESTURE_AI";

static HandDetect *s_detector = nullptr;
static HandGestureRecognizer *s_recognizer = nullptr;
static uint32_t s_frame_count = 0;

static const struct {
    const char *name;
    gesture_type_t type;
} s_gesture_map[] = {
    {"one",     GESTURE_ONE},
    {"two",     GESTURE_TWO},
    {"three",   GESTURE_THREE},
    {"four",    GESTURE_FOUR},
    {"five",    GESTURE_FIVE},
    {"like",    GESTURE_LIKE},
    {"ok",      GESTURE_OK},
    {"call",    GESTURE_CALL},
    {"dislike", GESTURE_DISLIKE},
};

extern "C" void init_gesture_ai(void)
{
    s_detector   = new HandDetect();
    s_recognizer = new HandGestureRecognizer();
    ESP_LOGI(TAG, "Hand gesture recognition loaded (9 gestures)");
}

extern "C" gesture_type_t process_gesture_frame(uint8_t* pixels, int width, int height, gesture_result_t *result)
{
    if (!s_detector || !s_recognizer || !pixels) return GESTURE_NONE;

    s_frame_count++;
    if (s_frame_count % 10 != 0) return GESTURE_NONE;

    dl::image::img_t img = {
        .data = pixels,
        .width = (uint16_t)width,
        .height = (uint16_t)height,
        .pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB565
    };

    auto &hands = s_detector->run(img);
    if (hands.empty()) return GESTURE_NONE;

    auto gestures = s_recognizer->recognize(img, hands);
    if (gestures.empty()) return GESTURE_NONE;

    auto &res = gestures.front();
    const char *cat = res.cat_name ? res.cat_name : "no_hand";

    gesture_type_t gesture = GESTURE_NONE;
    for (int i = 0; i < (int)(sizeof(s_gesture_map) / sizeof(s_gesture_map[0])); i++) {
        if (strcmp(cat, s_gesture_map[i].name) == 0) {
            gesture = s_gesture_map[i].type;
            break;
        }
    }

    if (result) {
        result->type = gesture;
        result->name = cat;
        result->confidence = res.score;
        auto &hand = hands.front();
        result->box_x1 = std::max(0, (int)hand.box[0]);
        result->box_y1 = std::max(0, (int)hand.box[1]);
        result->box_x2 = std::min(width - 1, (int)hand.box[2]);
        result->box_y2 = std::min(height - 1, (int)hand.box[3]);
    }

    if (gesture != GESTURE_NONE) {
        ESP_LOGI(TAG, "Gesture: %s [%s] (conf=%.2f)", gesture_type_to_str(gesture), cat, res.score);
    }

    return gesture;
}

extern "C" const char* gesture_type_to_str(gesture_type_t type)
{
    switch (type) {
        case GESTURE_ONE:     return "One (1)";
        case GESTURE_TWO:     return "Two (2)";
        case GESTURE_THREE:   return "Three (3)";
        case GESTURE_FOUR:    return "Four (4)";
        case GESTURE_FIVE:    return "Five (5)";
        case GESTURE_LIKE:    return "Like";
        case GESTURE_OK:      return "OK";
        case GESTURE_CALL:    return "Call";
        case GESTURE_DISLIKE: return "Dislike";
        default:              return "None";
    }
}
