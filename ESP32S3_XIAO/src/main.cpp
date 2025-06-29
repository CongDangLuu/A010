#include "esp_camera.h"
#include <WiFi.h>
#include <vector>
#include <string>
#include <iostream>

// #define ESP32_BLE
// #define ESP32_WIFI
#define ESP32_A010

#if defined(ESP32_BLE)
#include <XiaoBleServer.h>
XiaoBleServer bleServer;
#endif

#if defined(ESP32_A010)
#include <A010.h>
HardwareSerial a101Serial(1); // Create a HardwareSerial object for UART1
A010 a010(a101Serial);
#endif

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  Serial.println("[setup] ***********************************");
  delay(2000);

  #if defined(ESP32_BLE)
  bleServer.setOnDataReceived([](const std::string& data) {
    Serial.print("Received over BLE: ");
    Serial.println(data.c_str());
  });

  bleServer.init();
  #endif

  #if defined(ESP32_A010)
  a010.config(5, 5); // FPS = 5, Display = 5: UART & LCD
  #endif
}

void loop() {
  Serial.println("[Main] ***********************************");

  #if defined(ESP32_BLE)
  // Check BLE connection status
  bleServer.checkConnection();
  static unsigned long lastSend = 0;
  if (millis() - lastSend > 2000) {
    bleServer.sendMessage(String(millis()));
    lastSend = millis();
  }

  delay(100); // Small delay to prevent watchdog issues
  #endif

  #if defined(ESP32_A010)
  Serial.print("  [ESP32_A010] ------ ");
  std::vector<uint8_t> frame_a010 = a010.takePicture(1000);
  Serial.println("frame_a010.size() = " + String(frame_a010.size()));
  #endif
}
