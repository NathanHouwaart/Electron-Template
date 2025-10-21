// pn532CommandInterface.h
#pragma once
#include <string_view>
#include <vector>
#include "pn532Command.h"   // for pn532Response

namespace NFC_Controller::Cpp
{
    struct CommandRequest {
        uint8_t commandCode;                 // e.g. 0x4A for InListPassiveTarget
        std::vector<uint8_t> payload;        // data payload (excludes TFI)
        bool useExtendedFrame = false;       // when > 255 bytes or special cases
        uint32_t responseTimeoutMs = 0;      // optional override (0 = default)
    };

    struct CommandResult {
        pn532Response::statusCode status;
        std::vector<uint8_t> responsePayload;  // post-parsed payload bytes
    };

    class IPn532Command {
    public:
        virtual ~IPn532Command() = default;

        virtual std::string_view name() const noexcept = 0;
        virtual CommandRequest buildRequest() const = 0;
        virtual CommandResult parseResponse(const pn532Response& frame) const = 0;

        // Override if a command expects only the PN532 ACK frame.
        virtual bool expectsDataFrame() const noexcept { return true; }
    };
}