#pragma once // XIAOBLESERVERCALLBACK_H

#include <Arduino.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
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
