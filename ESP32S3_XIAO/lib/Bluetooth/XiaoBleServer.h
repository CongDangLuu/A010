#pragma once // BLESERVICE_H

#define ESP32_BLE

#if defined(ESP32_BLE)
#include <Arduino.h>

#include <esp_gap_ble_api.h>
#include <esp_gatts_api.h>
#include <esp_bt_defs.h>
#include <esp_bt_main.h>

#include "XiaoBleServerCallback.h"
#include "XiaoBleCharCallbacks.h"

// #define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
// #define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
// #define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
#define SERVICE_UUID           "0000fff0-0000-1000-8000-00805f9b34fb" // UART service UUID
#define CHARACTERISTIC_UUID_RX "0000fff1-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_UUID_TX "0000fff2-0000-1000-8000-00805f9b34fb"

// New characteristics for fragment transfer
#define CHARACTERISTIC_UUID_CTRL "0000fff3-0000-1000-8000-00805f9b34fb" // Control channel
#define CHARACTERISTIC_UUID_DATA "0000fff4-0000-1000-8000-00805f9b34fb" // Data channel

#define MAX_BYTE_SEND_PER_TIME 400

// Feature toggle macros
#define ENABLE_ACKNOWLEDGMENT 1  // Set to 0 to disable acknowledgment feature
#define ENABLE_CHECKSUM 1        // Set to 0 to disable checksum validation
#define ENABLE_RETRY 1           // Set to 0 to disable retry mechanism

// Transfer protocol constants
#define START_MARKER_1 0xAA
#define START_MARKER_2 0x55
#define START_MARKER_3 0xAA
#define ACK_MARKER 0x06
#define NACK_MARKER 0x15

// Transfer states
enum TransferState {
    IDLE,
    SENDING_HEADER,
    WAITING_HEADER_ACK,
    SENDING_FRAGMENT,
    WAITING_FRAGMENT_ACK,
    RETRYING_FRAGMENT,
    COMPLETED,
    ERROR
};

// Fragment header structure
struct FragmentHeader {
    uint16_t fragmentIndex;
    uint16_t fragmentSize;
    uint8_t isLast;
} __attribute__((packed));

// Image transfer header structure
struct ImageHeader {
    uint8_t startMarker1;
    uint8_t startMarker2;
    uint8_t startMarker3;
    uint32_t imageSize;
    uint16_t totalFragments;
    uint8_t imageFormat;
    uint8_t flags;  // Bit 0: ACK enabled, Bit 1: Checksum enabled, Bit 2: Retry enabled
    uint16_t checksum;
} __attribute__((packed));

// Acknowledgment structure
struct Acknowledgment {
    uint8_t ackMarker;
    uint16_t fragmentIndex;
    uint8_t status;
} __attribute__((packed));

class XiaoBleServer {
private:
    BLEServer* pServer = nullptr;
    BLECharacteristic* pTxCharacteristic = nullptr;
    BLECharacteristic* pCtrlCharacteristic = nullptr;
    BLECharacteristic* pDataCharacteristic = nullptr;
    XiaoBleServerCallback serverCallback;
    XiaoBleCharCallbacks characteristicCallback;

    bool oldDeviceConnected = false;
    bool deviceConnected = false;
    
    // Transfer state management
    TransferState transferState = IDLE;
    uint32_t imageSize = 0;
    uint16_t totalFragments = 0;
    uint16_t currentFragment = 0;
    uint8_t retryCount = 0;
    uint32_t lastSendTime = 0;
    uint32_t timeoutDuration = 500; // 500ms timeout
    
    // Transfer configuration
    bool ackEnabled = ENABLE_ACKNOWLEDGMENT;
    bool checksumEnabled = ENABLE_CHECKSUM;
    bool retryEnabled = ENABLE_RETRY;
    
    // Image data
    uint8_t* imageBuffer = nullptr;
    uint32_t imageBufferSize = 0;
    
    // Callbacks
    std::function<void(const std::string&)> onDataReceivedCallback;
    std::function<void(bool success)> onTransferCompleteCallback;

public:
    XiaoBleServer();
    ~XiaoBleServer();
    void init();
    void checkConnection();
    void sendMessage(const String& message);
    void sendImage(uint8_t* raw, size_t length);
    void setOnDataReceived(std::function<void(const std::string&)> callback);
    void setOnTransferComplete(std::function<void(bool success)> callback);
    
    // New fragment transfer methods
    bool startImageTransfer(uint8_t* imageData, uint32_t size, uint8_t format = 0x01);
    void processTransfer();
    void handleAcknowledgment(const std::string& data);
    uint16_t calculateCRC16(const uint8_t* data, size_t length);
    bool isTransferInProgress();
    void cancelTransfer();
    
    // Configuration methods
    void setAcknowledgmentEnabled(bool enabled);
    void setChecksumEnabled(bool enabled);
    void setRetryEnabled(bool enabled);
    bool isAcknowledgmentEnabled() const;
    bool isChecksumEnabled() const;
    bool isRetryEnabled() const;
    
private:
    void sendHeader();
    void sendFragment();
    void sendAcknowledgment(uint16_t fragmentIndex, uint8_t status);
    bool waitForAcknowledgment();
    void resetTransferState();
    uint8_t getTransferFlags();
};

#endif // ESP32_BLE
