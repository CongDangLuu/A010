#pragma once // A010.h

#include <vector>
#include <cstdint>
#include <Arduino.h>
#include <HardwareSerial.h>

// Debug message macros with log levels
#ifndef A010_DEBUG_LEVEL
#define A010_DEBUG_LEVEL 2  // Default debug level: 0=OFF, 1=ERROR, 2=WARN, 3=INFO, 4=DEBUG, 5=VERBOSE
#endif

// Debug level definitions
#define A010_LOG_LEVEL_OFF     0
#define A010_LOG_LEVEL_ERROR   1
#define A010_LOG_LEVEL_WARN    2
#define A010_LOG_LEVEL_INFO    3
#define A010_LOG_LEVEL_DEBUG   4
#define A010_LOG_LEVEL_VERBOSE 5

// Debug message macros
#define A010_LOG_ERROR(fmt, ...) \
    do { if (A010_DEBUG_LEVEL >= A010_LOG_LEVEL_ERROR) { Serial.print("[A010][ERROR] "); Serial.printf(fmt, ##__VA_ARGS__); Serial.println(); } } while(0)

#define A010_LOG_WARN(fmt, ...) \
    do { if (A010_DEBUG_LEVEL >= A010_LOG_LEVEL_WARN) { Serial.print("[A010][WARN]  "); Serial.printf(fmt, ##__VA_ARGS__); Serial.println(); } } while(0)

#define A010_LOG_INFO(fmt, ...) \
    do { if (A010_DEBUG_LEVEL >= A010_LOG_LEVEL_INFO) { Serial.print("[A010][INFO]  "); Serial.printf(fmt, ##__VA_ARGS__); Serial.println(); } } while(0)

#define A010_LOG_DEBUG(fmt, ...) \
    do { if (A010_DEBUG_LEVEL >= A010_LOG_LEVEL_DEBUG) { Serial.print("[A010][DEBUG] "); Serial.printf(fmt, ##__VA_ARGS__); Serial.println(); } } while(0)

#define A010_LOG_VERBOSE(fmt, ...) \
    do { if (A010_DEBUG_LEVEL >= A010_LOG_LEVEL_VERBOSE) { Serial.print("[A010][VERB]  "); Serial.printf(fmt, ##__VA_ARGS__); Serial.println(); } } while(0)

// Simple debug print without formatting
#define A010_LOG_ERROR_MSG(msg) \
    do { if (A010_DEBUG_LEVEL >= A010_LOG_LEVEL_ERROR) { Serial.print("[A010][ERROR] "); Serial.println(msg); } } while(0)

#define A010_LOG_WARN_MSG(msg) \
    do { if (A010_DEBUG_LEVEL >= A010_LOG_LEVEL_WARN) { Serial.print("[A010][WARN]  "); Serial.println(msg); } } while(0)

#define A010_LOG_INFO_MSG(msg) \
    do { if (A010_DEBUG_LEVEL >= A010_LOG_LEVEL_INFO) { Serial.print("[A010][INFO]  "); Serial.println(msg); } } while(0)

#define A010_LOG_DEBUG_MSG(msg) \
    do { if (A010_DEBUG_LEVEL >= A010_LOG_LEVEL_DEBUG) { Serial.print("[A010][DEBUG] "); Serial.println(msg); } } while(0)

#define A010_LOG_VERBOSE_MSG(msg) \
    do { if (A010_DEBUG_LEVEL >= A010_LOG_LEVEL_VERBOSE) { Serial.print("[A010][VERB]  "); Serial.println(msg); } } while(0)

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
