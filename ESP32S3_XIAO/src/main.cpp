#include "esp_camera.h"
#include <WiFi.h>

#define ESP32_BLE
// #define ESP32_WIFI

#define ARDUHAL_LOG_LEVEL 1

#if defined(ESP32_BLE)
#include <XiaoBleServer.h>
#include <Camera.h>

//init Bluetooth server
XiaoBleServer bleServer;

//init camera
Cam cam;

// Button
constexpr gpio_num_t PIN_BUTTON = GPIO_NUM_0;   // need confirm
constexpr uint32_t   DEBOUNCE_MS = 20;
/* ---- Button state ---- */
bool     lastBtnState = true;    // TRUE = OFF(PULL‑UP)
uint32_t lastDebounceTime = 0;
#endif

#ifdef ESP32_WIFI
// WiFi credentials
const char* ssid = "TODO";     // wifi name
const char* password = "TODO";  // password

// Static IP configuration
IPAddress staticIP(192, 168, 1, 15);    // static IP
IPAddress gateway(192, 168, 1, 1);       // gateway address
IPAddress subnet(255, 255, 255, 0);      // subnet mark

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

void startCameraServer();
#endif

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  pinMode(PIN_BUTTON, INPUT_PULLUP); 

  delay(2000);

  #if defined(ESP32_WIFI)
  Serial.println("Starting Camera initialization ...");

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // Camera quality settings
  if (psramFound()) {
    Serial.println("PSRAM found, setting high quality camera config");
    config.frame_size = FRAMESIZE_VGA;  // Change to VGA resolution
    config.jpeg_quality = 12;           // Adjust JPEG quality
    config.fb_count = 2;
  } else {
    Serial.println("No PSRAM found, setting lower quality camera config");
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  Serial.println("Camera initialized successfully");

  // Set initial frame size
  sensor_t * s = esp_camera_sensor_get();
  if(s) {
    s->set_framesize(s, FRAMESIZE_VGA);    // VGA resolution
    s->set_quality(s, 10);                 // Slightly improve quality
    s->set_brightness(s, 1);               // Moderate brightness (-2 to 2)
    s->set_contrast(s, 1);                 // Moderate contrast (-2 to 2)
    s->set_saturation(s, 0);              // Standard saturation (-2 to 2)
    s->set_whitebal(s, 1);                // Enable auto white balance
    s->set_awb_gain(s, 1);                // Enable auto white balance gain
    s->set_wb_mode(s, 2);                 // Use daylight mode (0: Auto 1: Sunny 2: Cloudy 3: Office 4: Home)
    s->set_gain_ctrl(s, 1);               // Keep auto gain
    s->set_exposure_ctrl(s, 1);           // Keep auto exposure
    s->set_aec2(s, 1);                    // Keep automatic exposure control
    s->set_ae_level(s, 0);                // Standard exposure level
    s->set_aec_value(s, 300);             // Lower automatic exposure value
    s->set_gainceiling(s, GAINCEILING_2X);// Further reduce the gain ceiling
    s->set_raw_gma(s, 1);                 // Enable gamma correction
    s->set_lenc(s, 1);                    // Enable lens correction
    Serial.println("Camera parameters adjusted for better color balance");
  }

  // Configuring WiFi
  WiFi.mode(WIFI_STA);        // set to STANDART mode
  WiFi.config(staticIP, gateway, subnet);  // config static IP
  WiFi.begin(ssid, password); // start connecting

  // wifi connecting
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");

  IPAddress IP = WiFi.localIP();
  Serial.println("----------------------------------------");
  Serial.println("WiFi Connected");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("IP Address: ");
  Serial.println(IP);
  Serial.println("----------------------------------------");

  // Test camera capture
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
  } else {
    Serial.printf("Captured image: width=%d, height=%d\n", fb->width, fb->height);
    esp_camera_fb_return(fb);
  }

  startCameraServer();
  Serial.println("Camera server started");
  Serial.print("Camera stream available at: http://");
  Serial.println(IP);
  Serial.println("----------------------------------------");
  #endif

  #if defined(ESP32_BLE)
  bleServer.setOnDataReceived([](const std::string& data) {
    Serial.print("Received over BLE: ");
    Serial.println(data.c_str());
  });

  bleServer.init();


  // Camerainit
  if (!cam.begin()) {
        Serial.println("Cam init failed, halt");
        while (true) delay(1000);
    }
  #endif
}

void loop() {
  #if defined(ESP32_WIFI)
  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost. Reconnecting...");
    WiFi.reconnect();
    delay(1000);
  }
  #endif

  #if defined(ESP32_BLE)
  // Check BLE connection status
  bleServer.checkConnection();
  // static unsigned long lastSend = 0;
  // if (millis() - lastSend > 2000) {
  //   bleServer.sendMessage(String(millis()));
  //   lastSend = millis();
  // }

  /* ----- read buttion with debounce ----- */
    bool rawState = digitalRead(PIN_BUTTON);          // HIGH/LOW
    if (rawState != lastBtnState) {                   // Statement of Buttion is changed
        lastDebounceTime = millis();                  // reset debounce time
        lastBtnState = rawState;
    }

    if ((millis() - lastDebounceTime) > DEBOUNCE_MS) {
        if (rawState == LOW) {                        // Button is pressed
            // Camera part
          //Get frame of camera
          camera_fb_t *fb = cam.capture();
          if (!fb) {
            Serial.println("[Main] Capture failed");
            return;
          }
          // Send the image via Bluetooth -----
          uint32_t len = fb->len;
          bleServer.sendMessage(String(len)); // send the length(4 byte)
          bleServer.sendMessage(String(*(fb->buf)));                        // send JPEG

          cam.returnFrame(fb);            // return buffer
          cam.setFrameDelay(2000);        // set delay
          delay(cam.frameDelay()); 
            /* waitting release Button*/
          while (digitalRead(PIN_BUTTON) == LOW) { delay(10); }
        }
    }

  
  delay(100); // Small delay to prevent watchdog issues
  #endif
}
