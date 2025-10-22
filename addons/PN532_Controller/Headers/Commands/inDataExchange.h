#pragma once

#include "../command.h"
#include "../hex.h"
#include <vector>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace NFC_Controller::Cpp
{
    /**
     * @brief Status byte values returned by InDataExchange command
     * @details First byte of the response indicates the result of the card communication
     */
    enum class InDataExchangeStatus : uint8_t {
        Success = 0x00,                     // Success
        Timeout = 0x01,                     // Time Out, the target has not answered
        CrcError = 0x02,                    // A CRC error has been detected by the CIU
        ParityError = 0x03,                 // A Parity error has been detected by the CIU
        ErroneousBitCount = 0x04,          // During anti-collision/select, erroneous Bit Count detected
        MifareFramingError = 0x05,         // Framing error during Mifare operation
        BitCollisionError = 0x06,          // Abnormal bit-collision during bit wise anti-collision
        BufferSizeInsufficient = 0x07,     // Communication buffer size insufficient
        RfBufferOverflow = 0x09,           // RF Buffer overflow detected by the CIU
        RfFieldNotSwitched = 0x0A,         // RF field has not been switched on in time (active mode)
        RfProtocolError = 0x0B,            // RF Protocol error
        TemperatureError = 0x0D,           // Temperature error
        InternalBufferOverflow = 0x0E,     // Internal buffer overflow
        InvalidParameter = 0x10,           // Invalid parameter
        DepCommandNotSupported = 0x12,     // DEP Protocol: command not supported in target mode
        DataFormatMismatch = 0x13,         // Data format does not match specification
        AuthenticationError = 0x14,        // Mifare: Authentication error
        UidCheckByteWrong = 0x23,          // ISO/IEC14443-3: UID Check byte is wrong
        InvalidDeviceState = 0x25,         // DEP Protocol: Invalid device state
        OperationNotAllowed = 0x26,        // Operation not allowed in this configuration
        CommandNotAcceptable = 0x27,       // Command not acceptable due to current context
        TargetReleased = 0x29,             // PN532 configured as target has been released
        CardIdMismatch = 0x2A,             // Card ID does not match (ISO/IEC14443-3B)
        CardDisappeared = 0x2B,            // Previously activated card has disappeared
        Nfcid3Mismatch = 0x2C,             // NFCID3 initiator/target mismatch in DEP 212/424 kbps
        OverCurrent = 0x2D,                // Over-current event detected
        NadMissing = 0x2E                  // NAD missing in DEP frame
    };

    /**
     * @brief Convert InDataExchangeStatus to human-readable string
     */
    inline const char* inDataExchangeStatusToString(InDataExchangeStatus status) {
        static const std::unordered_map<InDataExchangeStatus, const char*> statusMap = {
            {InDataExchangeStatus::Success, "Success"},
            {InDataExchangeStatus::Timeout, "Timeout - target has not answered"},
            {InDataExchangeStatus::CrcError, "CRC error detected by CIU"},
            {InDataExchangeStatus::ParityError, "Parity error detected by CIU"},
            {InDataExchangeStatus::ErroneousBitCount, "Erroneous Bit Count during anti-collision/select"},
            {InDataExchangeStatus::MifareFramingError, "Framing error during Mifare operation"},
            {InDataExchangeStatus::BitCollisionError, "Abnormal bit-collision during anti-collision"},
            {InDataExchangeStatus::BufferSizeInsufficient, "Communication buffer size insufficient"},
            {InDataExchangeStatus::RfBufferOverflow, "RF Buffer overflow detected"},
            {InDataExchangeStatus::RfFieldNotSwitched, "RF field not switched on in time"},
            {InDataExchangeStatus::RfProtocolError, "RF Protocol error"},
            {InDataExchangeStatus::TemperatureError, "Temperature error"},
            {InDataExchangeStatus::InternalBufferOverflow, "Internal buffer overflow"},
            {InDataExchangeStatus::InvalidParameter, "Invalid parameter"},
            {InDataExchangeStatus::DepCommandNotSupported, "DEP command not supported in target mode"},
            {InDataExchangeStatus::DataFormatMismatch, "Data format does not match specification"},
            {InDataExchangeStatus::AuthenticationError, "Mifare authentication error"},
            {InDataExchangeStatus::UidCheckByteWrong, "UID Check byte is wrong"},
            {InDataExchangeStatus::InvalidDeviceState, "Invalid device state"},
            {InDataExchangeStatus::OperationNotAllowed, "Operation not allowed in this configuration"},
            {InDataExchangeStatus::CommandNotAcceptable, "Command not acceptable in current context"},
            {InDataExchangeStatus::TargetReleased, "Target has been released by initiator"},
            {InDataExchangeStatus::CardIdMismatch, "Card ID does not match"},
            {InDataExchangeStatus::CardDisappeared, "Previously activated card has disappeared"},
            {InDataExchangeStatus::Nfcid3Mismatch, "NFCID3 initiator/target mismatch"},
            {InDataExchangeStatus::OverCurrent, "Over-current event detected"},
            {InDataExchangeStatus::NadMissing, "NAD missing in DEP frame"}
        };

        auto it = statusMap.find(status);
        return (it != statusMap.end()) ? it->second : "Unknown status";
    }

    /**
     * @brief Convert uint8_t status byte to InDataExchangeStatus enum
     */
    inline InDataExchangeStatus toInDataExchangeStatus(uint8_t statusByte) {
        return static_cast<InDataExchangeStatus>(statusByte);
    }

    /**
     * @brief InDataExchange command (0x40) - Exchange data with a target
     * @details This command sends data to a selected card/target and receives the response.
     * Used for ISO14443-4, ISO18092, and FeliCa data exchange.
     * 
     * Frame format:
     * - Tg (1 byte): Target number (typically 0x01 for single target)
     * - DataOut (variable): Data to send to the target
     * 
     * Response format:
     * - Status (1 byte): 0x00 = success, other = error
     * - DataIn (variable): Data received from the target
     */
    class InDataExchangeCommand final : public IPn532Command
    {
    public:
        struct Options {
            uint8_t targetNumber = 0x01;              // Target number (default 1)
            std::vector<uint8_t> payload;             // Data to send to card
            uint32_t responseTimeoutMs = 2000;        // Timeout for card response
        };

        explicit InDataExchangeCommand(Options opts)
            : options_(std::move(opts)) {}

        std::string_view name() const noexcept override {
            return "InDataExchange";
        }

        CommandRequest buildRequest() const override {
            CommandRequest req{};
            req.commandCode = pn532::command::InDataExchange;
            
            // Build payload: [Tg] [DataOut...]
            req.payload.reserve(1 + options_.payload.size());
            req.payload.push_back(options_.targetNumber);
            req.payload.insert(req.payload.end(), 
                             options_.payload.begin(), 
                             options_.payload.end());

            req.responseTimeoutMs = options_.responseTimeoutMs;
            return req;
        }

        CommandResult parseResponse(const pn532Response& frame) const override {
            CommandResult result{};
            result.status = frame.status;
            
            if (frame.status != pn532Response::statusCode::OK) {
                return result;
            }

            // InDataExchange response format: [Status] [DataIn...]
            // Status byte meanings:
            // 0x00: Success
            // 0x01: Time Out, the target has not answered
            // 0x02: A CRC error has been detected by the CIU
            // 0x03: A Parity error has been detected by the CIU
            // 0x04: During an anti-collision/select operation, an erroneous Bit Count has been detected
            // 0x05: Framing error during Mifare operation
            // 0x06: An abnormal bit-collision has been detected during bit wise anti-collision
            // 0x07: Communication buffer size insufficient
            // 0x09: RF Buffer overflow has been detected by the CIU
            // 0x0A: In active communication mode, the RF field has not been switched on in time
            // 0x0B: RF Protocol error
            // 0x0D: Temperature error
            // 0x0E: Internal buffer overflow
            // 0x10: Invalid parameter
            // 0x12: DEP Protocol: The PN532 configured in target mode does not support the command
            // 0x13: DEP Protocol / Mifare / ISO/IEC14443-4: The data format does not match to the specification
            // 0x14: Mifare: Authentication error
            // 0x23: ISO/IEC14443-3: UID Check byte is wrong
            // 0x25: DEP Protocol: Invalid device state
            // 0x26: Operation not allowed in this configuration
            // 0x27: This command is not acceptable due to the current context
            // 0x29: The PN532 configured as target has been released by its initiator
            // 0x2A: PN532 and ISO/IEC14443-3B only: the ID of the card does not match
            // 0x2B: PN532 and ISO/IEC14443-3B only: previous activated card has disappeared
            // 0x2C: Mismatch between the NFCID3 initiator and the NFCID3 target in DEP 212/424 kbps passive
            // 0x2D: An over-current event has been detected
            // 0x2E: NAD missing in DEP frame

            if (frame.length < 1) {
                result.status = pn532Response::statusCode::InvalidLength;
                return result;
            }

            uint8_t statusByte = frame.finalBuffer[0];
            
            // Store the status byte for caller inspection
            cachedStatusByte_ = statusByte;
            
            // NOTE: We always return OK from parseResponse() because the PN532 communication succeeded.
            // The card-level status is separate and should be checked via getStatus() or isSuccess().
            // This separation allows callers to distinguish between:
            // - Transport errors (PN532 communication failed)
            // - Card errors (card returned an error status like auth failure, timeout, etc.)
            
            // Extract data received from card (everything after status byte)
            if (frame.length > 1) {
                result.responsePayload.assign(frame.finalBuffer + 1,
                                            frame.finalBuffer + frame.length);
            }
            
            // Cache the response for getter access
            cachedResponse_ = result.responsePayload;
            
            return result;
        }

        // Getters for parsed data
        uint8_t getStatusByte() const { return cachedStatusByte_; }
        InDataExchangeStatus getStatus() const { 
            return toInDataExchangeStatus(cachedStatusByte_); 
        }
        const char* getStatusString() const { 
            return inDataExchangeStatusToString(getStatus()); 
        }
        bool isSuccess() const { 
            return cachedStatusByte_ == static_cast<uint8_t>(InDataExchangeStatus::Success); 
        }
        const std::vector<uint8_t>& getResponseData() const { return cachedResponse_; }

    private:
        Options options_;
        mutable uint8_t cachedStatusByte_ = 0xFF;
        mutable std::vector<uint8_t> cachedResponse_;
    };
}
