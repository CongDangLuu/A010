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

std::vector<uint8_t> A010::takePicture(uint32_t timeout_ms) {
  const uint8_t HEADER1 = 0x00;
  const uint8_t HEADER2 = 0xFF;
  const uint8_t FOOTER  = 0xDD;

  uint16_t packetLength = 0;
  uint16_t bytesRead = 0;
  bool inPacket = false;
  uint8_t buffer[4096];   // adjust depending on max packet size
  uint16_t bufPos = 0;

  uint8_t currentFrameSerial = 0;
  std::vector<uint8_t> frameBuffer;

  uint32_t startTime = millis();

  while (millis() - startTime < timeout_ms) {
    while (available()) {
      uint8_t b = read();

      if (!inPacket) {
        if (b == HEADER1) {
          if (available()) {
            uint8_t next = read();
            if (next == HEADER2) {
              inPacket = true;
              bytesRead = 0;
              bufPos = 0;
              continue;
            }
          }
        }
      } else {
        buffer[bufPos++] = b;
        bytesRead++;

        if (bufPos == 2 && packetLength == 0) {
          // after header, first 2 bytes = packet length
          packetLength = (buffer[0] << 8) | buffer[1];
        }

        if (bytesRead >= packetLength) {
          // entire packet arrived
          if (b == FOOTER) {
            // checksum check
            uint8_t sum = HEADER1 + HEADER2;
            for (int i = 0; i < bufPos - 1; i++) {
              sum += buffer[i];
            }
            if ((sum & 0xFF) == buffer[bufPos - 2]) {
              uint8_t packetSerial = buffer[2];  // metadata position, adjust if needed

              if (currentFrameSerial == 0) {
                currentFrameSerial = packetSerial;
              }

              if (packetSerial == currentFrameSerial) {
                // same frame, store payload
                // skip 16-byte metadata, skip checksum, skip footer
                uint16_t payloadLen = packetLength - 16 - 1 - 1;
                frameBuffer.insert(frameBuffer.end(),
                  buffer + 16, buffer + 16 + payloadLen);
              } else {
                // serial changed → means new frame started
                Serial.printf("[takeOneFrame] Got frame size: %d bytes\n", frameBuffer.size());
                return frameBuffer;
              }
            }
          }
          inPacket = false;
          packetLength = 0;
          bufPos = 0;
        }
      }
    }
    delay(1); // yield
  }

  // timeout
  if (!frameBuffer.empty()) {
    Serial.printf("[takeOneFrame] Timeout, partial frame size: %d bytes\n", frameBuffer.size());
  }
  return frameBuffer;
}