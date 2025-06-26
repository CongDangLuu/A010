#include "A010.h"

A010::A010(HardwareSerial &serial)
    : _serial(serial) {}   

size_t A010::write(uint8_t data) {
    return _serial.write(data);
}
void A010::config(int fps, int disp) {
    _serial.print("AT+FPS=" + String(fps) + "\r\n"); // Set frames per second
    delay(100);
    _serial.print("AT+DISP=" + String(disp) + "\r\n"); // Set display mode
    delay(100);
    _serial.print("AT+SAVE\r\n"); // Set display mode
}

int A010::available() {
    return _serial.available();
}
int A010::read() {
    return _serial.read();
}