#include "XiaoBleCharCallbacks.h"

// XiaoBleCharCallbacks class implementation
XiaoBleCharCallbacks::XiaoBleCharCallbacks() {}

XiaoBleCharCallbacks::~XiaoBleCharCallbacks() {}

void XiaoBleCharCallbacks::onWrite(BLECharacteristic *pCharacteristic) {
    std::string rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0)
    {
        Serial.println("*********");
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++)
            Serial.print(rxValue[i]);

        Serial.println();
        Serial.println("*********");

        if (onDataReceived) {
            onDataReceived(rxValue);
        } else {
            Serial.println("No callback set for data received.");
        }
    }
}

void XiaoBleCharCallbacks::setOnDataReceived(const DataCallback& callback) {
    onDataReceived = callback;
}