#pragma once // Camera.h
#include "esp_camera.h"

// XIAO ESP32S3 Camera configuration
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     10
#define SIOD_GPIO_NUM     40
#define SIOC_GPIO_NUM     39

#define Y9_GPIO_NUM       48
#define Y8_GPIO_NUM       11
#define Y7_GPIO_NUM       12
#define Y6_GPIO_NUM       14
#define Y5_GPIO_NUM       16
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM       17
#define Y2_GPIO_NUM       15
#define VSYNC_GPIO_NUM    38
#define HREF_GPIO_NUM     47
#define PCLK_GPIO_NUM     13


// XIAO ESP32‑S3 (Sense) pin map
static camera_config_t cam_config = {
    .pin_pwdn  = -1,
    .pin_reset = -1,
    .pin_xclk  = 10,

    .pin_sscb_sda = 11,
    .pin_sscb_scl = 12,

    .pin_d7 = 39, .pin_d6 = 40, .pin_d5 = 41, .pin_d4 = 42,
    .pin_d3 = 45, .pin_d2 = 46, .pin_d1 = 47, .pin_d0 = 48,

    .pin_vsync = 13,
    .pin_href  = 14,
    .pin_pclk  = 15,

    .xclk_freq_hz = 20000000,
    .ledc_timer   = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG,  // nén cứng trong phần cứng
    .frame_size   = FRAMESIZE_QQVGA, // 160×120; đổi QVGA 320×240 nếu băng thông đủ
    .jpeg_quality = 12,              // 0 – 63 (thấp = chất lượng cao)
    .fb_count     = 1,
    .grab_mode    = CAMERA_GRAB_LATEST
};
