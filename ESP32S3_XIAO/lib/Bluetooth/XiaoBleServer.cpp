#include "XiaoBleServer.h"

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

// XiaoBleCallbacks class implementation
XiaoBleCallbacks::XiaoBleCallbacks() {}

XiaoBleCallbacks::~XiaoBleCallbacks() {}

void XiaoBleCallbacks::onWrite(BLECharacteristic *pCharacteristic) {
    std::string rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0)
    {
        Serial.println("*********");
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++)
            Serial.print(rxValue[i]);

        Serial.println();
        Serial.println("*********");
    }
}

// XiaoBleServer class implementation
XiaoBleServer::XiaoBleServer() {
    pServer = nullptr;
    pTxCharacteristic = nullptr;
    serverCallback = new XiaoBleServerCallback();
    // characteristicCallback = new XiaoBleCallbacks();
}
XiaoBleServer::~XiaoBleServer() {
    if (pServer) {
        delete pServer;
        pServer = nullptr;
    }
    delete serverCallback;
    delete characteristicCallback;
}

void XiaoBleServer::init() {
    // Create the BLE Device
    BLEDevice::init("ESP32_BLE");

    // Set MTU size to avoid fragmentation issues
    BLEDevice::setMTU(23);

    // Create BLE server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(serverCallback);

    // Create the BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Create RX characteristic (for receiving data from phone)
    BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_WRITE_NR);
    pRxCharacteristic->setCallbacks(new XiaoBleCallbacks());

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
    deviceConnected = serverCallback->isDeviceConnected();
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