#include <Arduino.h>

#include <HardwareSerial.h>

// Define UART parameters
#define UART1_TX_PIN 43 
#define UART1_RX_PIN 44
#define BAUD_RATE_1 115200
#define BAUD_RATE_2 921600

HardwareSerial a101Serial(1); // Create a HardwareSerial object for UART1

void setup() {

  Serial.begin(BAUD_RATE_1);
  a101Serial.begin(BAUD_RATE_1, SERIAL_8N1, UART1_RX_PIN, UART1_TX_PIN);

  // Optional: Send initial commands to the camera
  a101Serial.print("AT+FPS=19\r");  // Set frames per second
  delay(100);
  a101Serial.print("AT+DISP=4\r");  // Set display mode
  delay(100);
}

void loop() {
// Pass data from the camera to the serial output
  while (a101Serial.available()) {
    Serial.write(a101Serial.read());  // Forward the camera's data to the serial monitor
  }
}

