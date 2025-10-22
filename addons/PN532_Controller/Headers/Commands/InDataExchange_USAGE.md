# InDataExchangeCommand Usage Examples

The `InDataExchangeCommand` class provides a clean, type-safe way to exchange data with NFC cards using the PN532's InDataExchange (0x40) command.

## Basic Usage

### Example 1: Send a DESFire GetVersion command

```cpp
#include "Commands/inDataExchange.h"

using namespace NFC_Controller::Cpp;

// Build DESFire GetVersion command in ISO 7816-4 wrapped format
// CLA=0x90 (proprietary), INS=0x60 (GetVersion), P1=0x00, P2=0x00, Le=0x00
std::vector<uint8_t> getVersionCmd = {0x90, 0x60, 0x00, 0x00, 0x00};

// Create the command
InDataExchangeCommand::Options opts;
opts.targetNumber = 0x01;           // Target 1 (first detected card)
opts.dataOut = getVersionCmd;       // Data to send
opts.responseTimeoutMs = 2000;      // 2 second timeout

auto command = InDataExchangeCommand(opts);

// Execute via PN532_chip
auto result = m_nfc_chip->executeCommand(command);

if (result.status == pn532Response::statusCode::OK) {
    // Check card status byte (first byte of response)
    uint8_t cardStatus = command.getStatusByte();
    if (cardStatus == 0x00) {
        // Success - get the data
        auto data = command.getResponseData();
        // data now contains the hardware info (7 bytes)
        std::cout << "Received " << data.size() << " bytes from card\n";
    } else {
        std::cerr << "Card returned error: 0x" << std::hex << (int)cardStatus << "\n";
    }
} else {
    std::cerr << "Command failed with status: " << (int)result.status << "\n";
}
```

### Example 2: DESFire SelectApplication

```cpp
// Build DESFire SelectApplication command
// Format: [CLA] [INS] [P1] [P2] [Lc] [AID(3)] [Le]
uint32_t aid = 0x010203;  // Application ID
std::vector<uint8_t> selectCmd = {
    0x90,                           // CLA
    (uint8_t)DesfireCommand::SELECT_APPLICATION,  // INS (0x5A)
    0x00, 0x00,                     // P1, P2
    0x03,                           // Lc (3 bytes of data)
    (uint8_t)(aid & 0xFF),          // AID byte 0
    (uint8_t)((aid >> 8) & 0xFF),   // AID byte 1
    (uint8_t)((aid >> 16) & 0xFF),  // AID byte 2
    0x00                            // Le
};

InDataExchangeCommand::Options opts;
opts.dataOut = selectCmd;
auto command = InDataExchangeCommand(opts);

auto result = m_nfc_chip->executeCommand(command);
if (result.status == pn532Response::statusCode::OK && 
    command.getStatusByte() == 0x00) {
    std::cout << "Application selected successfully\n";
}
```

### Example 3: DESFire Read Data

```cpp
// Read from file 1, offset 0, length 32 bytes
std::vector<uint8_t> readCmd = {
    0x90,                           // CLA
    (uint8_t)DesfireCommand::READ_DATA,  // INS (0xBD)
    0x00, 0x00,                     // P1, P2
    0x07,                           // Lc (7 bytes of data)
    0x01,                           // File number
    0x00, 0x00, 0x00,               // Offset (3 bytes, little-endian)
    0x20, 0x00, 0x00,               // Length (3 bytes, little-endian = 32)
    0x00                            // Le
};

InDataExchangeCommand::Options opts;
opts.dataOut = readCmd;
opts.responseTimeoutMs = 3000;  // Longer timeout for read operations
auto command = InDataExchangeCommand(opts);

auto result = m_nfc_chip->executeCommand(command);
if (result.status == pn532Response::statusCode::OK) {
    if (command.getStatusByte() == 0x00) {
        auto fileData = command.getResponseData();
        std::cout << "Read " << fileData.size() << " bytes from file\n";
        // Process fileData...
    }
}
```

### Example 4: Multi-frame response handling (like GetVersion)

```cpp
// DESFire GetVersion returns 3 frames
std::vector<uint8_t> cmd = {0x90, 0x60, 0x00, 0x00, 0x00};
uint8_t frameIndex = 0;

do {
    InDataExchangeCommand::Options opts;
    opts.dataOut = cmd;
    auto command = InDataExchangeCommand(opts);
    
    auto result = m_nfc_chip->executeCommand(command);
    if (result.status != pn532Response::statusCode::OK) break;
    
    auto data = command.getResponseData();
    
    // Check last 2 bytes for status (0x91 0xAF = more frames coming)
    if (data.size() >= 2) {
        uint8_t status1 = data[data.size() - 2];
        uint8_t status2 = data[data.size() - 1];
        
        // Remove status bytes from data
        data.resize(data.size() - 2);
        
        // Process this frame's data
        processFrame(frameIndex, data);
        
        // Check if more frames
        if (status1 == 0x91 && status2 == 0xAF) {
            // Send continuation command
            cmd[1] = (uint8_t)DesfireCommand::ADDITIONAL_FRAME;  // 0xAF
            frameIndex++;
        } else {
            break;  // Done
        }
    }
} while (frameIndex < 10);  // Safety limit
```

## Error Handling

The `InDataExchangeCommand` provides two levels of error checking:

1. **Transport-level errors**: Returned in `CommandResult.status`
   - `pn532Response::statusCode::OK`: PN532 communication succeeded
   - `pn532Response::statusCode::InvalidLength`: Malformed response
   - `pn532Response::statusCode::UnknownError`: PN532 or card error

2. **Card-level errors**: Returned in the first byte of the response (access via `getStatusByte()`)
   - `0x00`: Success
   - `0x01`: Timeout
   - `0x14`: Authentication error
   - `0x13`: Data format mismatch
   - (See full list in inDataExchange.h comments)

### Example: Complete error handling

```cpp
auto result = m_nfc_chip->executeCommand(command);

if (result.status != pn532Response::statusCode::OK) {
    std::cerr << "PN532 communication error: " << (int)result.status << "\n";
    return false;
}

uint8_t cardStatus = command.getStatusByte();
if (cardStatus != 0x00) {
    std::cerr << "Card operation failed with status: 0x" 
              << std::hex << (int)cardStatus << "\n";
    
    switch (cardStatus) {
        case 0x01: std::cerr << "Card timeout\n"; break;
        case 0x14: std::cerr << "Authentication failed\n"; break;
        case 0x13: std::cerr << "Invalid data format\n"; break;
        default: std::cerr << "Unknown card error\n"; break;
    }
    return false;
}

// Success - process response
auto data = command.getResponseData();
```

## Integration with MifareDesfireCard

The `InDataExchangeCommand` is designed to be used within card class methods:

```cpp
bool MifareDesfireCard::selectApplication(uint32_t aid) {
    if (!nfc_) return false;
    
    // Build command
    std::vector<uint8_t> cmd = {
        0x90, 0x5A, 0x00, 0x00, 0x03,
        (uint8_t)(aid & 0xFF),
        (uint8_t)((aid >> 8) & 0xFF),
        (uint8_t)((aid >> 16) & 0xFF),
        0x00
    };
    
    // Execute via new command interface
    InDataExchangeCommand::Options opts;
    opts.dataOut = cmd;
    auto command = InDataExchangeCommand(opts);
    
    auto result = static_cast<PN532_chip*>(nfc_)->executeCommand(command);
    
    return result.status == pn532Response::statusCode::OK && 
           command.getStatusByte() == 0x00;
}
```

## Benefits over raw initDataExchange

1. **Type safety**: Compile-time checked parameters
2. **Clear error handling**: Separate transport and card-level errors
3. **Consistent interface**: Matches other command classes
4. **Better testability**: Can mock `IPn532Command`
5. **Self-documenting**: Options struct makes intent clear
6. **Extensible**: Easy to add new options (retry logic, etc.)

## Next Steps

You can now use `InDataExchangeCommand` to implement DESFire operations in `MifareDesfireCard`:
- `selectApplication()`
- `authenticateAES()`
- `createApplication()`
- `readData()`
- `writeData()`
- `createStdDataFile()`
- etc.

Each method wraps the appropriate DESFire command in ISO 7816-4 format and uses `InDataExchangeCommand` to execute it.
