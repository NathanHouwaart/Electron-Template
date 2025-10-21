#pragma once
#include "../command.h"
#include "../card.h"
#include <algorithm>

namespace NFC_Controller::Cpp
{

    class InListPassiveTargetCommand final : public IPn532Command
    {
    public:

       

        struct Options{
            uint8_t maxTargets = 1;
            TargetType target = TargetType::TypeA_106kbps;
        };

        explicit InListPassiveTargetCommand(Options opts)
            : options_(std::move(opts)) {}

        std::string_view name() const noexcept override {
            return "InListPassiveTarget";
        }

        CommandRequest buildRequest() const override {
            CommandRequest req{};
            req.commandCode = pn532::command::InListPassiveTarget;
            req.payload = {options_.maxTargets, static_cast<uint8_t>(options_.target)};
            return req;
        }

        CommandResult parseResponse(const pn532Response& frame) const override {
            CommandResult result{};
            result.status = frame.status;
            
            if (frame.status != pn532Response::statusCode::OK) {
                return result;
            }

            if (frame.length < 1) {
                result.status = pn532Response::statusCode::InvalidLength;
                return result;
            }

            uint8_t nbTargets = frame.finalBuffer[0];
            detectedTargets_.clear();
            
            size_t index = 1;
            for (uint8_t i = 0; i < nbTargets; i++) {
                if (!parseTarget(frame, index, result)) {
                    return result; // Error already set in result.status
                }
            }

            return result;
        }

        const std::vector<TargetInfo>& getDetectedTargets() const {
            return detectedTargets_;
        }


    private:
        Options options_;
        mutable std::vector<TargetInfo> detectedTargets_;

        // Parse a single target from the response frame
        bool parseTarget(const pn532Response& frame, size_t& index, CommandResult& result) const {
            if (index >= frame.length) {
                Log("Index out of bounds while parsing target.\n");
                result.status = pn532Response::statusCode::InvalidLength;
                return false;
            }

            uint8_t targetNumber = frame.finalBuffer[index++];
            TargetInfo targetInfo{};

            bool success = (options_.target == TargetType::TypeA_106kbps)
                ? parseTypeATarget(frame, index, targetInfo, result)
                : parseOtherTarget(frame, index, targetInfo, result);

            if (!success) {
                return false;
            }

            detectedTargets_.push_back(targetInfo);
            populateResponsePayload(result, targetNumber, targetInfo);
            return true;
        }

        // Parse Type A card (ISO14443A) - most common type
        bool parseTypeATarget(const pn532Response& frame, size_t& index, 
                             TargetInfo& targetInfo, CommandResult& result) const {
            // Format: [ATQA(2)][SAK(1)][UIDLen(1)][UID...][ATSLen(1)][ATS...]
            if (index + 4 > frame.length) {
                Log("Index out of bounds while parseTypeATarget target.\n");
                result.status = pn532Response::statusCode::InvalidLength;
                return false;
            }

            // ATQA (SENS_RES) - 2 bytes, little endian
            targetInfo.atqa = static_cast<uint16_t>(frame.finalBuffer[index]) |
                             (static_cast<uint16_t>(frame.finalBuffer[index + 1]) << 8);
            index += 2;

            // SAK (SEL_RES)
            targetInfo.sak = frame.finalBuffer[index++];

            // UID
            if (!parseUID(frame, index, targetInfo, result)) {
                return false;
            }

            // ATS (optional)
            parseATS(frame, index, targetInfo);
            return true;
        }

        // Parse UID field
        bool parseUID(const pn532Response& frame, size_t& index, 
                     TargetInfo& targetInfo, CommandResult& result) const {
            if (index >= frame.length) {
                Log("Index out of bounds while parseTypeATarget target.\n");
                result.status = pn532Response::statusCode::InvalidLength;
                return false;
            }

            uint8_t uidLength = frame.finalBuffer[index++];
            
            if (index + uidLength > frame.length) {
                Log("Index out of bounds while parsing UID.\n");
                result.status = pn532Response::statusCode::InvalidLength;
                return false;
            }

            targetInfo.uid.assign(frame.finalBuffer + index,
                                 frame.finalBuffer + index + uidLength);
            index += uidLength;
            return true;
        }

        // Parse ATS (Answer To Select) - optional for ISO-DEP cards
        void parseATS(const pn532Response& frame, size_t& index, TargetInfo& targetInfo) const {
            // Only parse ATS if card supports ISO-DEP (SAK bit 5 set) and data is available
            if (index >= frame.length || (targetInfo.sak & 0x20) == 0) {
                return;
            }

            // ATS length byte (TL) includes itself in the count per ISO 14443-4
            uint8_t atsLength = frame.finalBuffer[index++];
            uint8_t dataBytes = (atsLength > 0) ? (atsLength - 1) : 0;
            
            if (dataBytes > 0 && index + dataBytes <= frame.length) {
                targetInfo.ats.assign(frame.finalBuffer + index, frame.finalBuffer + index + dataBytes);
                index += dataBytes;
            }
        }

        // Parse other card types (FeliCa, Type B, Jewel)
        bool parseOtherTarget(const pn532Response& frame, size_t& index,
                             TargetInfo& targetInfo, CommandResult& result) const {
            if (index + 1 > frame.length) {
                result.status = pn532Response::statusCode::InvalidLength;
                return false;
            }

            uint8_t dataLength = frame.finalBuffer[index++];
            
            if (index + dataLength > frame.length) {
                result.status = pn532Response::statusCode::InvalidLength;
                return false;
            }

            targetInfo.uid.assign(frame.finalBuffer + index,
                                 frame.finalBuffer + index + dataLength);
            index += dataLength;
            return true;
        }

        // Populate responsePayload for backward compatibility
        void populateResponsePayload(CommandResult& result, uint8_t targetNumber, 
                                    const TargetInfo& targetInfo) const {
            result.responsePayload.push_back(targetNumber);
            result.responsePayload.insert(result.responsePayload.end(),
                                         targetInfo.uid.begin(),
                                         targetInfo.uid.end());
        }
    };

} // namespace NFC_Controller::Cpp