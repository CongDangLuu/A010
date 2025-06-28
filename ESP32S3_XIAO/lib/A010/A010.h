#pragma once // A010.h

#include <vector>
#include <cstdint>
#include <Arduino.h>
#include <HardwareSerial.h>

class A010 {
public:
    A010(HardwareSerial &serial);

    size_t write(uint8_t data);
    void config(int fps = 19, int disp = 5);
    int available();
    int read();
    std::vector<uint8_t> takePicture(uint32_t timeout_ms = 2000);

private:
    HardwareSerial &_serial;
    uint32_t _baudRate;
};
