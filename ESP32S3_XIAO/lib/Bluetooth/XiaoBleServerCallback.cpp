#include "XiaoBleServerCallback.h"

// XiaoBleServerCallback class implementation
XiaoBleServerCallback::XiaoBleServerCallback() {}

XiaoBleServerCallback::~XiaoBleServerCallback() {}

void XiaoBleServerCallback::onConnect(BLEServer *pServer) {
    deviceConnected = true;
    Serial.println("Device connected");
}

void XiaoBleServerCallback::onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
    Serial.println("Device disconnected");
    // Restart advertising when device disconnects
    pServer->startAdvertising();
}

bool XiaoBleServerCallback::isDeviceConnected() const {
    return deviceConnected;
}