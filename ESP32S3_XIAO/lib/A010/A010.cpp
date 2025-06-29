#include "A010.h"

// Define UART parameters
#define UART1_TX_PIN 43 
#define UART1_RX_PIN 44

#define BAUD_RATE_A010 115200

A010::A010(HardwareSerial &serial)
    : _serial(serial) {
      A010_LOG_INFO("Initializing A010 with baud rate: %d", BAUD_RATE_A010);
      _serial.begin(BAUD_RATE_A010, SERIAL_8N1, UART1_RX_PIN, UART1_TX_PIN);
      A010_LOG_DEBUG("A010 serial communication initialized");
    }   

size_t A010::write(uint8_t data) {
    A010_LOG_VERBOSE("Writing byte: 0x%02X", data);
    return _serial.write(data);
}
void A010::config(int fps, int disp) {
  A010_LOG_INFO("Configuring A010 - FPS: %d, Display: %d", fps, disp);
  
  _serial.print("AT+ISP=0\r\n");  // Set display mode
  A010_LOG_DEBUG_MSG("Sent: AT+ISP=0");
  delay(100);
  _serial.print("AT+DISP=0\r\n");  // Set display mode
  A010_LOG_DEBUG_MSG("Sent: AT+DISP=0");
  delay(100);

  _serial.print("AT+ISP=1\r\n");  // Set display mode
  A010_LOG_DEBUG_MSG("Sent: AT+ISP=1");
  delay(100);
  _serial.print("AT+BINN=" + String(4) + "\r\n"); // Set display mode: 25x25
  A010_LOG_DEBUG("Sent: AT+BINN=%d", 4);
  delay(100);
  _serial.print("AT+FPS=" + String(fps) + "\r\n"); // Set frames per second
  A010_LOG_DEBUG("Sent: AT+FPS=%d", fps);
  delay(100);
  _serial.print("AT+DISP=" + String(disp) + "\r\n"); // Set display mode
  A010_LOG_DEBUG("Sent: AT+DISP=%d", disp);
  delay(100);
  // _serial.print("AT+SAVE\r\n");
  A010_LOG_INFO_MSG("A010 configuration completed");
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
  const uint16_t MAX_PACKET_SIZE = 4096;

  uint16_t packetLength = 0;
  uint16_t bytesRead = 0;
  bool inPacket = false;
  uint8_t buffer[MAX_PACKET_SIZE];
  uint16_t bufPos = 0;

  uint8_t currentFrameSerial = 0;
  std::vector<uint8_t> frameBuffer;

  uint32_t startTime = millis();
  
  A010_LOG_INFO_MSG("Starting frame capture");

  while (millis() - startTime < timeout_ms) {
    while (available()) {
      uint8_t b = read();

      if (!inPacket) {
        if (b == HEADER1) {
          A010_LOG_DEBUG_MSG("Found HEADER1");
          if (available()) {
            uint8_t next = read();
            if (next == HEADER2) {
              A010_LOG_DEBUG_MSG("Found HEADER2 - Starting packet capture");
              inPacket = true;
              bytesRead = 0;
              bufPos = 0;
              packetLength = 0;
              continue;
            } else {
              A010_LOG_WARN_MSG("Expected HEADER2 but got different byte - Resetting");
            }
          }
        }
      } else {
        // Check for buffer overflow
        if (bufPos >= MAX_PACKET_SIZE) {
          A010_LOG_ERROR_MSG("Buffer overflow! Resetting");
          inPacket = false;
          packetLength = 0;
          bufPos = 0;
          continue;
        }
        
        buffer[bufPos++] = b;
        bytesRead++;

        if (bufPos == 2 && packetLength == 0) {
          // after header, first 2 bytes = packet length
          packetLength = (buffer[1] << 8) | buffer[0];
          A010_LOG_DEBUG("Packet length calculated: %d", packetLength);
          
          // Validate packet length
          if (packetLength < 4 || packetLength > MAX_PACKET_SIZE) {
            A010_LOG_ERROR("Invalid packet length: %d - Resetting", packetLength);
            inPacket = false;
            packetLength = 0;
            bufPos = 0;
            continue;
          }
        }

        if ((bytesRead > 2) &&(bytesRead >= (packetLength + 2 + 2))) {
          A010_LOG_DEBUG("Packet complete - bytesRead: %d, packetLength: %d", bytesRead, packetLength);
          // entire packet arrived
          if (b == FOOTER) {
            A010_LOG_DEBUG("Found FOOTER: 0x%02X", b);
            // checksum check - only include buffer contents
            uint8_t sum = 0;
            for (int i = 0; i < bufPos - 2; i++) {
              sum += buffer[i];
              // A010_LOG_VERBOSE("0x%02X ", buffer[i]);
            }
            // Add header to checksum
            sum += (HEADER1 + HEADER2);
            uint8_t expectedChecksum = buffer[bufPos - 2];
            // A010_LOG_DEBUG("Expected checksum: 0x%02X", expectedChecksum);
            if ((sum & 0xFF) == expectedChecksum) {
              A010_LOG_DEBUG_MSG("Checksum OK");
              
              frameBuffer.insert(frameBuffer.end(), buffer + 18, buffer + 18 + packetLength - 16);
              A010_LOG_INFO("Frame buffer size: %d", frameBuffer.size());
              return frameBuffer;

            } else {
              A010_LOG_ERROR_MSG("Checksum FAILED - Discarding packet");
            }
          } else {
            A010_LOG_WARN("Expected FOOTER but got different byte: 0x%02X - Invalid packet", b);
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
  A010_LOG_WARN_MSG("Timeout reached");
  if (!frameBuffer.empty()) {
    A010_LOG_WARN_MSG("Timeout, partial frame received");
  } else {
    A010_LOG_ERROR_MSG("No data received during timeout period");
  }
  return frameBuffer;
}