#include "XiaoBleServer.h"

// XiaoBleServer class implementation
XiaoBleServer::XiaoBleServer() {}
XiaoBleServer::~XiaoBleServer() {}

void XiaoBleServer::init() {
    // Create the BLE Device
    BLEDevice::init("ESP32_BLE");

    // Set MTU size to avoid fragmentation issues
    BLEDevice::setMTU(23);

    // Create BLE server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(&serverCallback);

    // Create the BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Create RX characteristic (for receiving data from phone)
    BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_WRITE_NR);
    pRxCharacteristic->setCallbacks(&characteristicCallback);

    // Create TX characteristic (for sending data to phone)
    pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_NOTIFY);
    pTxCharacteristic->addDescriptor(new BLE2902());

    // Set initial value for TX characteristic
    pTxCharacteristic->setValue("Hello from ESP32!");

    // Start the service
    pService->start();

    // Configure advertising with simpler settings
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinInterval(0x20); // 20ms
    pAdvertising->setMaxInterval(0x40); // 40ms

    // Start advertising
    BLEDevice::startAdvertising();
    Serial.println("BLE Server started and advertising");
    Serial.println("Device name: ESP32_BLE");
    Serial.println("Service UUID: " + String(SERVICE_UUID));
    Serial.println("RX Characteristic UUID: " + String(CHARACTERISTIC_UUID_RX));
    Serial.println("TX Characteristic UUID: " + String(CHARACTERISTIC_UUID_TX));
    Serial.println("Waiting for connections...");
}

void XiaoBleServer::checkConnection() {
    deviceConnected = serverCallback.isDeviceConnected();
    if (deviceConnected && !oldDeviceConnected) {
        // New connection
        oldDeviceConnected = deviceConnected;
        Serial.println("New device connected! \n");
    }

    if (!deviceConnected && oldDeviceConnected){
        // Device disconnected
        oldDeviceConnected = deviceConnected;
        Serial.println("Device disconnected! \n");
        delay(500); // Give the BLE stack a chance to get an event
    }
}

void XiaoBleServer::sendMessage(const String& message) {
    if (!deviceConnected) return; 

    String mess = "ESP32 send: " + message;
    pTxCharacteristic->setValue(mess.c_str());
    pTxCharacteristic->notify();
    Serial.println("Sent: " + mess); 
}

void XiaoBleServer::setOnDataReceived(std::function<void(const std::string&)> callback) {
    characteristicCallback.setOnDataReceived(callback);
}