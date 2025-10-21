// card.h
#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <sstream>
#include <iomanip>

// Shared target information structure used by both InListPassiveTarget and Card classes
struct TargetInfo {
    std::vector<uint8_t> uid;   // Unique Identifier (4, 7, or 10 bytes)
    uint16_t atqa = 0;          // Answer To Request (Type A) - stored as uint16_t for consistency
    uint8_t sak = 0;            // Select Acknowledge
    std::vector<uint8_t> ats;   // Optional, Type A cards (from RATS)
};

class Card {
public:
    explicit Card(TargetInfo id)
        : id_(std::move(id)) {}

    virtual ~Card() = default;

    // ---- Accessors ----
    const std::vector<uint8_t>& uid() const noexcept { return id_.uid; }
    uint16_t atqa() const noexcept { return id_.atqa; }
    uint8_t sak() const noexcept { return id_.sak; }
    const std::vector<uint8_t>& ats() const noexcept { return id_.ats; }

    // ---- Utility ----
    virtual std::string prettyType() const = 0;
    virtual std::string toString() const;

protected:
    TargetInfo id_;
};

// Inline default implementation so the symbol is available to linkers.
inline std::string Card::toString() const {
    std::ostringstream oss;
    oss << prettyType() << " ";
    oss << "UID:";
    for (size_t i = 0; i < id_.uid.size(); ++i) {
        if (i) oss << ":";
        oss << std::hex << std::setw(2) << std::setfill('0') << (static_cast<int>(id_.uid[i]) & 0xFF);
    }
    oss << std::dec << " ATQA:0x" << std::hex << id_.atqa;
    oss << std::dec << " SAK:0x" << std::hex << static_cast<int>(id_.sak);
    return oss.str();
}
