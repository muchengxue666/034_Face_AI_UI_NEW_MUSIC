#include "face_ai.h"
#include "human_face_detect.hpp"
#include "human_face_recognition.hpp" 
#include "dl_image_define.hpp"
#include "esp_log.h"
#include "esp_cache.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <algorithm>
#include <list>
#include <stdio.h>
#include "mqtt_app.h" // 引入 MQTT 发报机

static const char *TAG = "FACE_AI";

static HumanFaceDetect *detector = nullptr;
static HumanFaceRecognizer *recognizer = nullptr; 

static uint32_t frame_counter = 0; 
static bool is_master_enrolled = false; 

// 记忆盒子与容忍度
static int last_x1 = 0, last_y1 = 0, last_x2 = 0, last_y2 = 0;
static bool has_face = false;
static int patience_counter = 0; 

extern "C" void init_face_ai(void) {
    detector = new HumanFaceDetect(); 
    recognizer = new HumanFaceRecognizer("", (HumanFaceFeat::model_type_t)1, false); 
    ESP_LOGI(TAG, "🤖 AI 大脑 (后台无感版) 加载完毕！");
}

extern "C" void process_face_frame(uint8_t* pixels, int width, int height) {
    if (!detector || !recognizer) return;
    
    frame_counter++;

    dl::image::img_t img = {
        .data = pixels,
        .width = (uint16_t)width,
        .height = (uint16_t)height,
        .pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB565
    };
    
    // ---------------------------------------------------------------------
    // 💡 第一部分：AI 运算
    // ---------------------------------------------------------------------
    if (frame_counter % 5 == 0) {
        auto &results = detector->run(img);
        
        if (results.size() > 0) {
            has_face = true; 
            patience_counter = 0; 
            
            auto &box = results.front(); 
            
            last_x1 = std::max(0, std::min((int)box.box[0], width - 1));
            last_y1 = std::max(0, std::min((int)box.box[1], height - 1));
            last_x2 = std::max(0, std::min((int)box.box[2], width - 1));
            last_y2 = std::max(0, std::min((int)box.box[3], height - 1));

            std::list<dl::detect::result_t> single_face_list;
            single_face_list.push_back(box);

            if (!is_master_enrolled) {
                recognizer->enroll(img, single_face_list);
                auto verify_list = recognizer->recognize(img, single_face_list);
                
                if (verify_list.size() > 0 && verify_list.front().id > 0) {
                    ESP_LOGW(TAG, "🧠 记忆写入成功！我已经死死记住主人的脸了！");
                    is_master_enrolled = true; 
                } else {
                    ESP_LOGE(TAG, "❌ 录入失败！请物理旋转摄像头！端正坐姿，不要歪头！");
                }
            } else {
                auto rec_res_list = recognizer->recognize(img, single_face_list);
                
                if (rec_res_list.size() > 0) {
                    auto &rec_res = rec_res_list.front();
                    if (rec_res.id > 0) {
                        ESP_LOGI(TAG, "✅ 身份确认：主人归来！(相似度: %.2f)", rec_res.similarity);
                        
                        // 💡 联动 MQTT 发送数据
                        char payload[128];
                        snprintf(payload, sizeof(payload), "{\"event\":\"master_detected\", \"similarity\":%.2f}", rec_res.similarity);
                        sensehub_publish_message("sensehub/kitchen/vision", payload);
                        
                    } else {
                        ESP_LOGE(TAG, "🚨 警报：发现陌生人！！！(相似度: %.2f)", rec_res.similarity);
                    }
                } else {
                    ESP_LOGW(TAG, "⚠️ 看到脸了，但太模糊或侧脸，AI 无法确认身份...");
                }
            } 
        } else {
            patience_counter++;
            if (patience_counter > 3) { 
                has_face = false;
            }
        }
    }

    // ---------------------------------------------------------------------
    // 💡 第二部分：极速画笔 (已移除)
    // 因为画面不再推送到屏幕，这里的画框和 Cache 同步纯属浪费算力，直接剥离。
    // ---------------------------------------------------------------------
}