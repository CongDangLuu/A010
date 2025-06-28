#include "Camera.h"

Cam::Cam(camera_config_t *cfg, uint16_t interDelayMs)
    : _cfg(cfg), _frameDelay(interDelayMs) {}

bool Cam::begin()
{
    Serial.println("[Cam] Initialising camera…");
    if (esp_camera_init(_cfg) != ESP_OK) {
        Serial.println("[Cam] Camera init failed");
        return false;
    }
    Serial.println("[Cam] Camera OK");
    return true;
}

camera_fb_t *Cam::capture()
{
    return esp_camera_fb_get();   // có thể trả về nullptr
}

void Cam::returnFrame(camera_fb_t *fb)
{
    if (fb) esp_camera_fb_return(fb);
}

void Cam::setFrameDelay(uint16_t ms)
{
    _frameDelay = ms;
}
