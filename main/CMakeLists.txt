idf_component_register(
    SRC_DIRS
        "."
        "APP/MIPI_CAM"
        "APP/FACE_AI"     # 人脸识别文件夹
        "APP/MQTT"        # MQTT 文件夹
        "APP/WIFI_APP"    # WiFi 模块文件夹
        "APP/DOUBAO_AI"   # 豆包大模型文件夹
        "APP/VOICE"       # 语音流水线文件夹
        "APP/UI"          # UI 文件夹
        "APP/AUDIO"       # 音频处理文件夹
    INCLUDE_DIRS
        "."
        "APP/MIPI_CAM"
        "APP/FACE_AI"
        "APP/MQTT"
        "APP/WIFI_APP"
        "APP/DOUBAO_AI"
        "APP/VOICE"       # 语音流水线头文件路径
        "APP/UI"
        "APP/AUDIO"
    REQUIRES
        BSP
        esp_wifi
        esp_event
        nvs_flash
        esp_http_client   # 豆包通信必须要用到它
        esp-tls           # HTTPS 加密证书需要
        esp_driver_ppa
        esp_video         # 视频底层
        esp-dl            # 核心通行证 1：深度学习底层算子
        human_face_detect         # 核心通行证 2：人脸检测引擎
        human_face_recognition    # 核心通行证 3：人脸识别引擎
        hand_gesture_recognition  # 手势识别引擎
        hand_detect               # 手部检测引擎
        mqtt
        json              # CJSON 解析库
        lvgl              # LVGL
        esp_lvgl_port     # ESP 官方的 LVGL 移植库
        esp_codec_dev     # 音频编解码器抽象层
        MYFATFS           # FATFS 扩展组件（提供 exfuns.h）
        spiffs            # SPIFFS 文件系统（存放 GIF 等资源）
        zlib              # gzip 解压支持（天气接口返回压缩体）
)