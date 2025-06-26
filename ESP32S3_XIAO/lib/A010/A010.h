#pragma once // A010.h

#include <Arduino.h>
#include <HardwareSerial.h>

class A010 {
public:
    A010(HardwareSerial &serial);

    size_t write(uint8_t data);
    void config(int fps = 19, int disp = 5);
    int available();
    int read();

private:
    HardwareSerial &_serial;
    uint32_t _baudRate;
};
