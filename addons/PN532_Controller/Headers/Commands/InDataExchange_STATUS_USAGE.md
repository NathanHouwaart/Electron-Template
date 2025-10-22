# InDataExchange Status Enum Usage

The `InDataExchangeStatus` enum and helper functions provide type-safe, readable error handling for card communication.

## Status Enum

```cpp
enum class InDataExchangeStatus : uint8_t {
    Success = 0x00,
    Timeout = 0x01,
    CrcError = 0x02,
    ParityError = 0x03,
    // ... (27 total status codes)
    AuthenticationError = 0x14,
    CardDisappeared = 0x2B,
    // ...
};
```

## Usage Examples

### Example 1: Basic status checking with enum

```cpp
#include "Commands/inDataExchange.h"

using namespace NFC_Controller::Cpp;

// Execute command
InDataExchangeCommand::Options opts;
opts.dataOut = {0x90, 0x60, 0x00, 0x00, 0x00};  // DESFire GetVersion
auto command = InDataExchangeCommand(opts);

auto result = m_nfc_chip->executeCommand(command);

if (result.status == pn532Response::statusCode::OK) {
    // Check card status using enum
    auto status = command.getStatus();
    
    switch (status) {
        case InDataExchangeStatus::Success:
            std::cout << "Card operation succeeded!\n";
            auto data = command.getResponseData();
            // Process data...
            break;
            
        case InDataExchangeStatus::Timeout:
            std::cerr << "Card did not respond in time\n";
            break;
            
        case InDataExchangeStatus::AuthenticationError:
            std::cerr << "Authentication failed - wrong key?\n";
            break;
            
        case InDataExchangeStatus::CardDisappeared:
            std::cerr << "Card was removed during operation\n";
            break;
            
        default:
            std::cerr << "Card error: " << command.getStatusString() << "\n";
            break;
    }
}
```

### Example 2: Using helper methods

```cpp
// Simple success check
if (command.isSuccess()) {
    std::cout << "Operation successful!\n";
    processData(command.getResponseData());
}

// Get human-readable error message
if (!command.isSuccess()) {
    std::cerr << "Card operation failed: " 
              << command.getStatusString() << "\n";
    std::cerr << "Status code: 0x" 
              << std::hex << (int)command.getStatusByte() << "\n";
}
```

### Example 3: Logging with descriptive messages

```cpp
void logCardOperation(const InDataExchangeCommand& cmd, const std::string& operation) {
    auto status = cmd.getStatus();
    
    if (status == InDataExchangeStatus::Success) {
        std::cout << "[SUCCESS] " << operation << " completed\n";
    } else {
        std::cerr << "[ERROR] " << operation << " failed: "
                  << cmd.getStatusString() 
                  << " (0x" << std::hex << (int)cmd.getStatusByte() << ")\n";
    }
}

// Usage
InDataExchangeCommand selectCmd(/*...*/);
m_nfc_chip->executeCommand(selectCmd);
logCardOperation(selectCmd, "SelectApplication(0x010203)");
```

### Example 4: Error handling in DESFire methods

```cpp
bool MifareDesfireCard::selectApplication(uint32_t aid) {
    if (!nfc_) return false;
    
    std::vector<uint8_t> cmd = {
        0x90, 0x5A, 0x00, 0x00, 0x03,
        (uint8_t)(aid & 0xFF),
        (uint8_t)((aid >> 8) & 0xFF),
        (uint8_t)((aid >> 16) & 0xFF),
        0x00
    };
    
    InDataExchangeCommand::Options opts;
    opts.dataOut = cmd;
    auto command = InDataExchangeCommand(opts);
    
    auto result = static_cast<PN532_chip*>(nfc_)->executeCommand(command);
    
    if (result.status != pn532Response::statusCode::OK) {
        std::cerr << "PN532 communication failed\n";
        return false;
    }
    
    // Use enum for clear error handling
    switch (command.getStatus()) {
        case InDataExchangeStatus::Success:
            return true;
            
        case InDataExchangeStatus::AuthenticationError:
            std::cerr << "Application requires authentication first\n";
            return false;
            
        case InDataExchangeStatus::InvalidParameter:
            std::cerr << "Application ID 0x" << std::hex << aid 
                      << " does not exist\n";
            return false;
            
        default:
            std::cerr << "SelectApplication failed: " 
                      << command.getStatusString() << "\n";
            return false;
    }
}
```

### Example 5: Converting raw status bytes

```cpp
// If you have a raw status byte from somewhere
uint8_t rawStatus = 0x14;

// Convert to enum
auto status = toInDataExchangeStatus(rawStatus);

// Get string description
const char* description = inDataExchangeStatusToString(status);

std::cout << "Status 0x" << std::hex << (int)rawStatus 
          << ": " << description << "\n";
// Output: "Status 0x14: Mifare authentication error"
```

## All Available Methods

```cpp
class InDataExchangeCommand {
    // Get raw status byte (0x00-0xFF)
    uint8_t getStatusByte() const;
    
    // Get typed enum value
    InDataExchangeStatus getStatus() const;
    
    // Get human-readable description
    const char* getStatusString() const;
    
    // Quick success check
    bool isSuccess() const;
    
    // Get response data (empty if error)
    const std::vector<uint8_t>& getResponseData() const;
};
```

## Status Code Reference

| Code | Enum | Description |
|------|------|-------------|
| 0x00 | Success | Operation completed successfully |
| 0x01 | Timeout | Target has not answered |
| 0x02 | CrcError | CRC error detected by CIU |
| 0x03 | ParityError | Parity error detected by CIU |
| 0x04 | ErroneousBitCount | Erroneous bit count during anti-collision |
| 0x05 | MifareFramingError | Framing error during Mifare operation |
| 0x06 | BitCollisionError | Abnormal bit-collision detected |
| 0x07 | BufferSizeInsufficient | Communication buffer too small |
| 0x09 | RfBufferOverflow | RF Buffer overflow detected |
| 0x0A | RfFieldNotSwitched | RF field not switched on in time |
| 0x0B | RfProtocolError | RF Protocol error |
| 0x0D | TemperatureError | Temperature error |
| 0x0E | InternalBufferOverflow | Internal buffer overflow |
| 0x10 | InvalidParameter | Invalid parameter |
| 0x12 | DepCommandNotSupported | DEP command not supported |
| 0x13 | DataFormatMismatch | Data format mismatch |
| 0x14 | AuthenticationError | Mifare authentication error |
| 0x23 | UidCheckByteWrong | UID Check byte is wrong |
| 0x25 | InvalidDeviceState | Invalid device state |
| 0x26 | OperationNotAllowed | Operation not allowed |
| 0x27 | CommandNotAcceptable | Command not acceptable |
| 0x29 | TargetReleased | Target released by initiator |
| 0x2A | CardIdMismatch | Card ID does not match |
| 0x2B | CardDisappeared | Card has disappeared |
| 0x2C | Nfcid3Mismatch | NFCID3 mismatch |
| 0x2D | OverCurrent | Over-current event detected |
| 0x2E | NadMissing | NAD missing in DEP frame |

## Benefits

1. **Type Safety**: Compile-time checking of status values
2. **Readability**: `InDataExchangeStatus::AuthenticationError` vs `0x14`
3. **Self-Documenting**: Clear intent in switch statements
4. **Easy Logging**: `getStatusString()` provides instant descriptions
5. **No Magic Numbers**: All status codes are named constants
