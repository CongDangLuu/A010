
#pragma once // XIAOBLECHARCALLBACKS_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <functional>

class XiaoBleCharCallbacks : public BLECharacteristicCallbacks  {
public:
    XiaoBleCharCallbacks();
    ~XiaoBleCharCallbacks();
    void onWrite(BLECharacteristic *pCharacteristic);

    // Callback for when data is received
    using DataCallback = std::function<void(const std::string&)>;
    void setOnDataReceived(const DataCallback& callback);
private:
    DataCallback onDataReceived;
};
