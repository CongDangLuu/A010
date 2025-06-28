#ifndef CAMERA_H
#define CAMERA_H

#include <Arduino.h>
#include "esp_camera.h"
#include "camera_pins.h"   // chứa cam_config cho XIAO ESP32‑S3

class Cam {
public:
    explicit Cam(camera_config_t *cfg = &cam_config,
                 uint16_t interDelayMs = 100);

    bool        begin();                       // khởi tạo camera
    camera_fb_t *capture();                    // lấy khung JPEG (nullptr nếu lỗi)
    void        returnFrame(camera_fb_t *fb);  // trả buffer cho driver
    void        setFrameDelay(uint16_t ms);
    uint16_t    frameDelay() const { return _frameDelay; }

private:
    camera_config_t *_cfg;
    uint16_t         _frameDelay;
};

#endif /* CAM_H */
