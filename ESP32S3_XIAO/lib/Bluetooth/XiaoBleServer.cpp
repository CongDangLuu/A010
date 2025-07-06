#include "XiaoBleServer.h"

// XiaoBleServer class implementation
XiaoBleServer::XiaoBleServer() {}
XiaoBleServer::~XiaoBleServer() {
    if (imageBuffer) {
        free(imageBuffer);
        imageBuffer = nullptr;
    }
}

void XiaoBleServer::init() {
    // Create the BLE Device
    BLEDevice::init("ESP32_BLE");

    // Set MTU size to avoid fragmentation issues
    BLEDevice::setMTU(512);

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

    // Create Control characteristic (for transfer control)
    pCtrlCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_CTRL,
        BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_NOTIFY);
    pCtrlCharacteristic->addDescriptor(new BLE2902());
    pCtrlCharacteristic->setCallbacks(&characteristicCallback);

    // Create Data characteristic (for fragment data)
    pDataCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_DATA,
        BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_NOTIFY);
    pDataCharacteristic->addDescriptor(new BLE2902());

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
    Serial.println("CTRL Characteristic UUID: " + String(CHARACTERISTIC_UUID_CTRL));
    Serial.println("DATA Characteristic UUID: " + String(CHARACTERISTIC_UUID_DATA));
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
        if (transferState != IDLE) {
            cancelTransfer();
        }
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

void XiaoBleServer::sendImage(uint8_t* raw, size_t length) {
    if (!deviceConnected) return; 
    
    pTxCharacteristic->setValue(raw, length);
    pTxCharacteristic->notify();
}

void XiaoBleServer::setOnDataReceived(std::function<void(const std::string&)> callback) {
    onDataReceivedCallback = callback;
    characteristicCallback.setOnDataReceived(callback);
}

void XiaoBleServer::setOnTransferComplete(std::function<void(bool success)> callback) {
    onTransferCompleteCallback = callback;
}

// Fragment transfer implementation
bool XiaoBleServer::startImageTransfer(uint8_t* imageData, uint32_t size, uint8_t format) {
    if (!deviceConnected || transferState != IDLE) {
        Serial.println("Cannot start transfer: not connected or transfer in progress");
        return false;
    }
    
    // Allocate buffer and copy image data
    if (imageBuffer) {
        free(imageBuffer);
    }
    imageBuffer = (uint8_t*)malloc(size);
    if (!imageBuffer) {
        Serial.println("Failed to allocate image buffer");
        return false;
    }
    
    memcpy(imageBuffer, imageData, size);
    imageBufferSize = size;
    imageSize = size;
    totalFragments = (size + MAX_BYTE_SEND_PER_TIME - 1) / MAX_BYTE_SEND_PER_TIME;
    currentFragment = 0;
    retryCount = 0;
    
    Serial.printf("Starting image transfer: %u bytes, %u fragments \r\n", size, totalFragments);
    Serial.printf("Features: ACK=%s, Checksum=%s, Retry=%s\r\n", 
                  ackEnabled ? "ON" : "OFF",
                  checksumEnabled ? "ON" : "OFF", 
                  retryEnabled ? "ON" : "OFF");
    
    transferState = SENDING_HEADER;
    sendHeader();
    
    return true;
}

void XiaoBleServer::processTransfer() {
    if (!deviceConnected || transferState == IDLE) {
        return;
    }
    
    uint32_t currentTime = millis();
    
    switch (transferState) {
        case SENDING_HEADER:
            // Header already sent, wait for acknowledgment if enabled
            if (ackEnabled) {
                transferState = WAITING_HEADER_ACK;
                lastSendTime = currentTime;
            } else {
                // Skip acknowledgment, go directly to sending fragments
                transferState = SENDING_FRAGMENT;
            }
            break;
            
        case WAITING_HEADER_ACK:
            if (currentTime - lastSendTime > timeoutDuration) {
                Serial.println("    Header acknowledgment timeout, retrying...");
                retryCount++;
                if (retryCount >= 3) {
                    Serial.println("    Header acknowledgment failed after 3 retries");
                    transferState = ERROR;
                    if (onTransferCompleteCallback) {
                        onTransferCompleteCallback(false);
                    }
                } else {
                    transferState = SENDING_HEADER;
                    sendHeader();
                }
            }
            break;
            
        case SENDING_FRAGMENT:
            sendFragment();
            if (ackEnabled) {
                transferState = WAITING_FRAGMENT_ACK;
                lastSendTime = currentTime;
            } else {
                // No acknowledgment needed, send next fragment or complete
                currentFragment++;
                if (currentFragment >= totalFragments) {
                    transferState = COMPLETED;
                } else {
                    // Continue with next fragment
                    delay(20); // Small delay between fragments
                }
            }
            break;
            
        case WAITING_FRAGMENT_ACK:
            if (currentTime - lastSendTime > timeoutDuration) {
                Serial.printf("    Fragment %u acknowledgment timeout, retrying...\r\n", currentFragment);
                retryCount++;
                if (retryCount >= 3) {
                    Serial.printf("    Fragment %u failed after 3 retries\r\n", currentFragment);
                    transferState = ERROR;
                    if (onTransferCompleteCallback) {
                        onTransferCompleteCallback(false);
                    }
                } else {
                    transferState = RETRYING_FRAGMENT;
                    sendFragment();
                    transferState = WAITING_FRAGMENT_ACK;
                    lastSendTime = currentTime;
                }
            }
            break;
            
        case COMPLETED:
            Serial.println("  Image transfer completed successfully");
            resetTransferState();
            if (onTransferCompleteCallback) {
                onTransferCompleteCallback(true);
            }
            break;
            
        case ERROR:
            Serial.println("  Image transfer failed");
            resetTransferState();
            if (onTransferCompleteCallback) {
                onTransferCompleteCallback(false);
            }
            break;
            
        default:
            break;
    }
}

void XiaoBleServer::handleAcknowledgment(const std::string& data) {
    if (!ackEnabled) {
        return; // Ignore acknowledgments if disabled
    }
    
    if (data.length() < sizeof(Acknowledgment)) {
        Serial.println("Invalid acknowledgment data length");
        return;
    }
    
    Acknowledgment* ack = (Acknowledgment*)data.data();
    
    if (ack->ackMarker != ACK_MARKER && ack->ackMarker != NACK_MARKER) {
        Serial.println("Invalid acknowledgment marker");
        return;
    }
    
    Serial.printf("Received acknowledgment: fragment=%u, status=%u\r\n", ack->fragmentIndex, ack->status);
    
    if (transferState == WAITING_HEADER_ACK) {
        if (ack->fragmentIndex == 0xFFFF && ack->status == 0x00) {
            // Header acknowledged successfully
            Serial.println("Header acknowledged, starting fragment transfer");
            transferState = SENDING_FRAGMENT;
            retryCount = 0;
        } else {
            Serial.println("Header acknowledgment failed");
            transferState = ERROR;
        }
    } else if (transferState == WAITING_FRAGMENT_ACK) {
        if (ack->fragmentIndex == currentFragment) {
            if (ack->status == 0x00) {
                // Fragment acknowledged successfully
                retryCount = 0;
                currentFragment++;
                
                if (currentFragment >= totalFragments) {
                    // All fragments sent
                    transferState = COMPLETED;
                } else {
                    // Send next fragment
                    transferState = SENDING_FRAGMENT;
                }
            } else {
                // Fragment error, retry if enabled
                Serial.printf("Fragment %u error, retrying...\r\n", currentFragment);
                if (retryEnabled) {
                    retryCount++;
                    if (retryCount >= 3) {
                        transferState = ERROR;
                    } else {
                        transferState = RETRYING_FRAGMENT;
                    }
                } else {
                    transferState = ERROR;
                }
            }
        }
    }
}

void XiaoBleServer::sendHeader() {
    if (!deviceConnected) return;
    
    ImageHeader header;
    header.startMarker1 = START_MARKER_1;
    header.startMarker2 = START_MARKER_2;
    header.startMarker3 = START_MARKER_3;
    header.imageSize = imageSize;
    header.totalFragments = totalFragments;
    header.imageFormat = 0x01; // JPEG
    header.flags = getTransferFlags();
    
    if (checksumEnabled) {
        header.checksum = calculateCRC16((uint8_t*)&header + 6, sizeof(header) - 6); // Exclude start markers
    } else {
        header.checksum = 0;
    }
    
    pCtrlCharacteristic->setValue((uint8_t*)&header, sizeof(header));
    pCtrlCharacteristic->notify();
    
    Serial.printf("Sent header: size=%u, fragments=%u, flags=0x%02X\r\n", imageSize, totalFragments, header.flags);
}

void XiaoBleServer::sendFragment() {
    if (!deviceConnected || currentFragment >= totalFragments) return;
    
    uint32_t offset = currentFragment * MAX_BYTE_SEND_PER_TIME;
    uint16_t fragmentSize = (offset + MAX_BYTE_SEND_PER_TIME > imageSize) ? 
                           (imageSize - offset) : MAX_BYTE_SEND_PER_TIME;
    
    // Calculate packet size
    size_t headerSize = sizeof(FragmentHeader);
    size_t checksumSize = checksumEnabled ? 2 : 0;
    size_t packetSize = headerSize + fragmentSize + checksumSize;
    
    // Create fragment packet
    uint8_t* packet = (uint8_t*)malloc(packetSize);
    if (!packet) {
        Serial.println("Failed to allocate fragment packet");
        return;
    }
    
    FragmentHeader* header = (FragmentHeader*)packet;
    header->fragmentIndex = currentFragment;
    header->fragmentSize = fragmentSize;
    header->isLast = (currentFragment == totalFragments - 1) ? 0x01 : 0x00;
    
    // Copy fragment data
    memcpy(packet + headerSize, imageBuffer + offset, fragmentSize);
    
    // Calculate and append checksum if enabled
    if (checksumEnabled) {
        uint16_t checksum = calculateCRC16(packet + headerSize, fragmentSize);
        packet[headerSize + fragmentSize] = checksum & 0xFF;
        packet[headerSize + fragmentSize + 1] = (checksum >> 8) & 0xFF;
    }
    
    pDataCharacteristic->setValue(packet, packetSize);
    pDataCharacteristic->notify();
    
    Serial.printf(" Sent fragment %u/%u: %u bytes\r\n", currentFragment + 1, totalFragments, fragmentSize);
    
    free(packet);
}

void XiaoBleServer::sendAcknowledgment(uint16_t fragmentIndex, uint8_t status) {
    if (!deviceConnected || !ackEnabled) return;
    
    Acknowledgment ack;
    ack.ackMarker = ACK_MARKER;
    ack.fragmentIndex = fragmentIndex;
    ack.status = status;
    
    pCtrlCharacteristic->setValue((uint8_t*)&ack, sizeof(ack));
    pCtrlCharacteristic->notify();
}

// Configuration methods
void XiaoBleServer::setAcknowledgmentEnabled(bool enabled) {
    ackEnabled = enabled;
    Serial.printf("Acknowledgment feature: %s\n", enabled ? "ENABLED" : "DISABLED");
}

void XiaoBleServer::setChecksumEnabled(bool enabled) {
    checksumEnabled = enabled;
    Serial.printf("Checksum feature: %s\n", enabled ? "ENABLED" : "DISABLED");
}

void XiaoBleServer::setRetryEnabled(bool enabled) {
    retryEnabled = enabled;
    Serial.printf("Retry feature: %s\n", enabled ? "ENABLED" : "DISABLED");
}

bool XiaoBleServer::isAcknowledgmentEnabled() const {
    return ackEnabled;
}

bool XiaoBleServer::isChecksumEnabled() const {
    return checksumEnabled;
}

bool XiaoBleServer::isRetryEnabled() const {
    return retryEnabled;
}

uint8_t XiaoBleServer::getTransferFlags() {
    uint8_t flags = 0;
    if (ackEnabled) flags |= 0x01;
    if (checksumEnabled) flags |= 0x02;
    if (retryEnabled) flags |= 0x04;
    return flags;
}

uint16_t XiaoBleServer::calculateCRC16(const uint8_t* data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc = crc >> 1;
            }
        }
    }
    return crc;
}

bool XiaoBleServer::isTransferInProgress() {
    return transferState != IDLE;
}

void XiaoBleServer::cancelTransfer() {
    Serial.println("Canceling image transfer");
    resetTransferState();
}

void XiaoBleServer::resetTransferState() {
    transferState = IDLE;
    currentFragment = 0;
    retryCount = 0;
    lastSendTime = 0;
    
    if (imageBuffer) {
        free(imageBuffer);
        imageBuffer = nullptr;
    }
    imageBufferSize = 0;
    imageSize = 0;
    totalFragments = 0;
}