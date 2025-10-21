#pragma once

#include "../command.h"
#include "../hex.h"

namespace NFC_Controller::Cpp
{
    struct FirmwareInfo {
        uint8_t ic;
        uint8_t ver;
        uint8_t rev;
        uint8_t supportFlags;

        void printInfo() const {
            std::cout << "Firmware Information:" << std::endl;
            std::cout << "IC: " << Hex0x(ic) << std::endl;
            std::cout << "Version: " << Hex0x(ver) << std::endl;
            std::cout << "Revision: " << Hex0x(rev) << std::endl;
            std::cout << "Support Flags: " << Hex0x(supportFlags) << std::endl;
        }
    };

    class GetFirmwareVersionCommand final : public IPn532Command {
    public:
        std::string_view name() const noexcept override {
            return "GetFirmwareVersion";
        }

        CommandRequest buildRequest() const override {
            CommandRequest req{};
            req.commandCode = pn532::command::GetFirmwareVersion;
            req.payload.clear();         // this command has no parameters
            return req;
        }

        CommandResult parseResponse(const pn532Response& frame) const override {

            CommandResult result{};
            result.status = frame.status;

            if (frame.status != pn532Response::statusCode::OK) {
                return result; // transport-layer issue
            }

            static constexpr uint8_t kExpectedPayloadLength = 4;
            static constexpr uint8_t kIndexIc      = 0;
            static constexpr uint8_t kIndexVersion = 1;
            static constexpr uint8_t kIndexRevision= 2;
            static constexpr uint8_t kIndexSupport = 3;

            if (frame.length != kExpectedPayloadLength) {
                result.status = pn532Response::statusCode::InvalidLength;
                return result;
            }

            FirmwareInfo info{
                frame.finalBuffer[kIndexIc],
                frame.finalBuffer[kIndexVersion],
                frame.finalBuffer[kIndexRevision],
                frame.finalBuffer[kIndexSupport]
            };

            // store raw payload for callers that just want bytes
            result.responsePayload.assign(frame.finalBuffer,
                                          frame.finalBuffer + frame.length);
            // optionally stash decoded info in a subclass-specific way
            cachedInfo = info;
            return result;
        }

        const FirmwareInfo& firmware() const { return cachedInfo; }

    private:
        mutable FirmwareInfo cachedInfo{};
    };
}
