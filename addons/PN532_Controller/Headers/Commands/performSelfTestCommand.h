// addons/PN532_Controller/Headers/Commands/PerformSelfTestCommand.h
#pragma once
#include "../command.h"
#include <algorithm>

namespace NFC_Controller::Cpp
{
    class PerformSelfTestCommand final : public IPn532Command
    {
    public:
        enum class Test : uint8_t {
            CommunicationLine = 0x00,
            RomChecksum       = 0x01,
            RamIntegrity      = 0x02,
            PollingToTarget   = 0x04,
            EchoBack          = 0x05,
            CardPresence      = 0x06,
            AntennaContinuity = 0x07
        };

        struct Options {
            Test test;
            std::vector<uint8_t> parameters{};   // optional NumTst-specific payload
            bool verifyEcho = true;              // applies to CommunicationLine
            uint32_t responseTimeoutMs = 0;      // 0 -> use default for selected test
        };

        explicit PerformSelfTestCommand(Options opts)
            : options_(std::move(opts)) {}

        std::string_view name() const noexcept override {
            return "PerformSelfTest";
        }

        CommandRequest buildRequest() const override {
            CommandRequest req{};
            req.commandCode = pn532::command::PerformSelftest;
            req.payload.reserve(1 + options_.parameters.size());
            req.payload.push_back(static_cast<uint8_t>(options_.test));
            req.payload.insert(req.payload.end(),
                               options_.parameters.begin(),
                               options_.parameters.end());
            req.responseTimeoutMs = options_.responseTimeoutMs != 0
                                        ? options_.responseTimeoutMs
                                        : defaultTimeoutFor(options_.test);
            return req;
        }

        CommandResult parseResponse(const pn532Response& frame) const override {
            CommandResult result{};
            result.status = frame.status;
            if (frame.status != pn532Response::statusCode::OK) {
                return result;                               // transport-level failure
            }

            result.responsePayload.assign(frame.finalBuffer,
                                          frame.finalBuffer + frame.length);

            switch (options_.test)
            {
            case Test::CommunicationLine:
                {
                    const std::size_t expectedSize = options_.parameters.size() + 1U;
                    bool payloadMatches = options_.parameters.empty();
                    if (!payloadMatches && result.responsePayload.size() == expectedSize)
                    {
                        payloadMatches = std::equal(options_.parameters.begin(),
                                                    options_.parameters.end(),
                                                    result.responsePayload.begin() + 1);
                    }
                    const bool validEcho =
                        result.responsePayload.size() == expectedSize &&
                        !result.responsePayload.empty() &&
                        result.responsePayload.front() == static_cast<uint8_t>(Test::CommunicationLine) &&
                        payloadMatches;
                    if (!validEcho && options_.verifyEcho)
                    {
                        result.status = pn532Response::statusCode::InvalidDataChecksum;
                    }
                }
                break;
            case Test::EchoBack:
                if (!result.responsePayload.empty())
                {
                    result.status = pn532Response::statusCode::InvalidLength;
                }
                break;
            default:
                if (result.responsePayload.empty())
                {
                    result.status = pn532Response::statusCode::InvalidLength;
                }
                break;
            }
            return result;
        }

        bool expectsDataFrame() const noexcept override {
            return options_.test != Test::EchoBack;          // NumTst 0x05 never returns
        }

        static constexpr uint8_t makeAntennaThreshold(uint8_t highThresholdCode,
                                                      uint8_t lowThresholdCode,
                                                      bool useUpperComparator,
                                                      bool useLowerComparator) noexcept
        {
            highThresholdCode &= 0x03;
            lowThresholdCode  &= 0x03;

            uint8_t value = 0;
            if (useLowerComparator)
            {
                value |= static_cast<uint8_t>(1u << 6); // andet_bot
            }
            if (useUpperComparator)
            {
                value |= static_cast<uint8_t>(1u << 5); // andet_up
            }

            value |= static_cast<uint8_t>(lowThresholdCode  << 3); // bits 4..3
            value |= static_cast<uint8_t>(highThresholdCode << 1); // bits 2..1
            value |= static_cast<uint8_t>(1u << 0);               // andet_en
            return value;
        }

    private:
        static constexpr uint32_t defaultTimeoutFor(Test test) noexcept
        {
            switch (test)
            {
            case Test::CommunicationLine:
                return 500;     // quick echo
            case Test::RomChecksum:
            case Test::RamIntegrity:
                return 2000;    // checksum / memory sweep
            case Test::PollingToTarget:
            case Test::EchoBack:
                return 4000;    // may involve RF retries / waits
            case Test::CardPresence:
            case Test::AntennaContinuity:
                return 2000;    // analog measurement
            default:
                return 5000;
            }
        }

        Options options_;
    };
}
