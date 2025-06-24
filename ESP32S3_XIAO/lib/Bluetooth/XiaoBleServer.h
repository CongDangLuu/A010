#ifndef BLESERVICE_H
#define BLESERVICE_H
#define ESP32_BLE


#if defined(ESP32_BLE)
#include <Arduino.h>

#include <esp_gap_ble_api.h>
#include <esp_gatts_api.h>
#include <esp_bt_defs.h>
#include <esp_bt_main.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// #define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
// #define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
// #define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
#define SERVICE_UUID           "0000fff0-0000-1000-8000-00805f9b34fb" // UART service UUID
#define CHARACTERISTIC_UUID_RX "0000fff1-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_UUID_TX "0000fff2-0000-1000-8000-00805f9b34fb"


class XiaoBleServerCallback : public BLEServerCallbacks {
private:
    bool deviceConnected = false;
public:
    XiaoBleServerCallback();
    ~XiaoBleServerCallback();

    void onConnect(BLEServer* pServer);
    void onDisconnect(BLEServer* pServer);

    bool isDeviceConnected() const;
};

class XiaoBleCallbacks : public BLECharacteristicCallbacks  {
private:
public:
    XiaoBleCallbacks();
    ~XiaoBleCallbacks();
    void onWrite(BLECharacteristic *pCharacteristic);
};

class XiaoBleServer {
private:
    BLEServer* pServer;
    BLECharacteristic* pTxCharacteristic;
    XiaoBleServerCallback* serverCallback;
    XiaoBleCallbacks* characteristicCallback;

    bool oldDeviceConnected = false;
    bool deviceConnected = false;
public:
    XiaoBleServer();
    ~XiaoBleServer();
    void init();
    void checkConnection();
    void sendMessage(const String& message);
};

#endif // ESP32_BLE
#endif // BLESERVICE_H


