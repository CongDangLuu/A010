# BLE Fragment Transfer Protocol

## Overview

This document describes the implementation of a reliable fragment-based image transfer protocol over BLE for the ESP32S3 XIAO camera project.

## Feature Toggles

The protocol includes configurable features that can be enabled/disabled:

### Compile-time Configuration (Macros)
```cpp
#define ENABLE_ACKNOWLEDGMENT 1  // Set to 0 to disable acknowledgment feature
#define ENABLE_CHECKSUM 1        // Set to 0 to disable checksum validation
#define ENABLE_RETRY 1           // Set to 0 to disable retry mechanism
```

### Runtime Configuration (Methods)
```cpp
bleServer.setAcknowledgmentEnabled(true/false);   // Enable/disable acknowledgments
bleServer.setChecksumEnabled(true/false);         // Enable/disable checksums
bleServer.setRetryEnabled(true/false);            // Enable/disable retries
```

## Protocol Design

### BLE Characteristics

The protocol uses four BLE characteristics:

1. **RX Characteristic** (`0000fff1-0000-1000-8000-00805f9b34fb`)
   - For receiving general data from the client
   - Handles acknowledgments and control messages

2. **TX Characteristic** (`0000fff2-0000-1000-8000-00805f9b34fb`)
   - For sending general messages to the client
   - Legacy compatibility

3. **Control Characteristic** (`0000fff3-0000-1000-8000-00805f9b34fb`)
   - For transfer control messages
   - Sends image headers and receives acknowledgments

4. **Data Characteristic** (`0000fff4-0000-1000-8000-00805f9b34fb`)
   - For sending image fragment data
   - Contains fragment headers and payload

### Transfer Protocol

#### 1. Image Header Packet
```
[START_MARKER_1][START_MARKER_2][START_MARKER_3][IMAGE_SIZE][TOTAL_FRAGMENTS][IMAGE_FORMAT][FLAGS][CHECKSUM]
```
- **Start Markers**: 0xAA, 0x55, 0xAA (fixed sequence)
- **Image Size**: 4 bytes (total image size in bytes)
- **Total Fragments**: 2 bytes (number of fragments)
- **Image Format**: 1 byte (0x01 = JPEG)
- **Flags**: 1 byte (Bit 0: ACK enabled, Bit 1: Checksum enabled, Bit 2: Retry enabled)
- **Checksum**: 2 bytes (CRC16 of header data, 0 if checksum disabled)

#### 2. Fragment Data Packet
```
[FRAGMENT_INDEX][FRAGMENT_SIZE][IS_LAST][FRAGMENT_DATA][CHECKSUM]
```
- **Fragment Index**: 2 bytes (0-based fragment number)
- **Fragment Size**: 2 bytes (actual data size in this fragment)
- **Is Last**: 1 byte (0x00 = more fragments, 0x01 = last fragment)
- **Fragment Data**: Actual image data (up to 400 bytes)
- **Checksum**: 2 bytes (CRC16 of fragment data, omitted if checksum disabled)

#### 3. Acknowledgment Packet (Only when ACK enabled)
```
[ACK_MARKER][FRAGMENT_INDEX][STATUS]
```
- **ACK Marker**: 0x06 (ACK) or 0x15 (NACK)
- **Fragment Index**: 2 bytes (which fragment was received)
- **Status**: 1 byte (0x00 = OK, 0x01 = Error, 0x02 = Request retry)

## Transfer Modes

### Mode 1: Full Reliability (Default)
- **Acknowledgment**: Enabled
- **Checksum**: Enabled
- **Retry**: Enabled
- **Use Case**: Critical applications requiring guaranteed delivery
- **Performance**: Slower but most reliable

### Mode 2: Fast Transfer
- **Acknowledgment**: Disabled
- **Checksum**: Enabled
- **Retry**: Disabled
- **Use Case**: Real-time applications where speed is priority
- **Performance**: Faster but less reliable

### Mode 3: Minimal Overhead
- **Acknowledgment**: Disabled
- **Checksum**: Disabled
- **Retry**: Disabled
- **Use Case**: High-speed streaming where some data loss is acceptable
- **Performance**: Fastest but least reliable

## Transfer Flow

### Sender (ESP32) Side:
1. **Initialize Transfer**: Call `startImageTransfer()`
2. **Send Header**: Send image metadata via Control characteristic
3. **Wait for Header ACK**: Wait for acknowledgment if ACK enabled, otherwise proceed
4. **Send Fragments**: Send each fragment sequentially
5. **Wait for Fragment ACK**: Wait for acknowledgment if ACK enabled, otherwise continue
6. **Retry on Failure**: Retry failed fragments if retry enabled
7. **Complete**: Mark transfer as complete

### Receiver (Client) Side:
1. **Receive Header**: Parse image metadata and flags
2. **Send Header ACK**: Acknowledge header if ACK enabled
3. **Receive Fragments**: Receive and validate each fragment
4. **Send Fragment ACK**: Acknowledge each fragment if ACK enabled
5. **Reassemble Image**: Combine fragments in correct order
6. **Validate Complete Image**: Check final image integrity

## Error Handling

### Timeout Management
- **Header Timeout**: 500ms (only when ACK enabled)
- **Fragment Timeout**: 500ms (only when ACK enabled)
- **Retry Delays**: Exponential backoff (100ms, 200ms, 400ms) (only when retry enabled)

### Error Recovery
- **Checksum Mismatch**: Request retry of specific fragment (only when ACK enabled)
- **Missing Fragment**: Request specific fragment by index (only when ACK enabled)
- **Connection Loss**: Restart transfer from beginning
- **Buffer Overflow**: Request smaller fragment size

## Usage Examples

### Example 1: Full Reliability Mode
```cpp
// Initialize with all features enabled (default)
bleServer.setAcknowledgmentEnabled(true);
bleServer.setChecksumEnabled(true);
bleServer.setRetryEnabled(true);

// Start image transfer
if (bleServer.startImageTransfer(imageBuffer, imageSize, 0x01)) {
    while (bleServer.isTransferInProgress()) {
        bleServer.processTransfer();
        delay(10);
    }
}
```

### Example 2: Fast Transfer Mode
```cpp
// Disable acknowledgments for faster transfer
bleServer.setAcknowledgmentEnabled(false);
bleServer.setChecksumEnabled(true);
bleServer.setRetryEnabled(false);

// Start image transfer
if (bleServer.startImageTransfer(imageBuffer, imageSize, 0x01)) {
    while (bleServer.isTransferInProgress()) {
        bleServer.processTransfer();
        delay(10);
    }
}
```

### Example 3: Minimal Overhead Mode
```cpp
// Disable all reliability features for maximum speed
bleServer.setAcknowledgmentEnabled(false);
bleServer.setChecksumEnabled(false);
bleServer.setRetryEnabled(false);

// Start image transfer
if (bleServer.startImageTransfer(imageBuffer, imageSize, 0x01)) {
    while (bleServer.isTransferInProgress()) {
        bleServer.processTransfer();
        delay(10);
    }
}
```

## Configuration

### Fragment Size
- **Default**: 400 bytes per fragment
- **Configurable**: Via `MAX_BYTE_SEND_PER_TIME` define
- **Adaptive**: Can be adjusted based on connection quality

### Timeout Settings
- **Header Timeout**: 500ms (only when ACK enabled)
- **Fragment Timeout**: 500ms (only when ACK enabled)
- **Overall Timeout**: 30 seconds

### Feature Flags
- **Bit 0**: Acknowledgment enabled (0x01)
- **Bit 1**: Checksum enabled (0x02)
- **Bit 2**: Retry enabled (0x04)

## Benefits

1. **Flexibility**: Choose reliability level based on application needs
2. **Performance**: Optimize for speed or reliability as needed
3. **Reliability**: Checksums and acknowledgments ensure data integrity when enabled
4. **Robustness**: Retry mechanisms handle temporary failures when enabled
5. **Efficiency**: Optimized timing and buffer management
6. **Compatibility**: Works with existing BLE infrastructure

## Limitations

1. **Sequential Transfer**: Fragments sent one at a time
2. **Memory Usage**: Requires buffer for complete image
3. **Latency**: Acknowledgment overhead increases total time when enabled
4. **Complexity**: More complex than simple streaming

## Future Improvements

1. **Parallel Transfer**: Send multiple fragments simultaneously
2. **Compression**: Add image compression before transfer
3. **Progressive Transfer**: Send low-res first, then high-res
4. **Adaptive Fragment Size**: Dynamically adjust based on connection quality
5. **Dynamic Feature Toggle**: Change features mid-transfer based on conditions 