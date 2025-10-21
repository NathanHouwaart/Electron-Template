#pragma once

#include "../card.h"
#include <cstdint>
#include <vector>

class MifareClassicCard : public Card {
public:
    virtual std::string prettyType() const override {
        return "MIFARE Classic";
    }

    MifareClassicCard(TargetInfo id)
        : Card(std::move(id)) {}

    virtual void authenticate(uint8_t blockNumber, const uint8_t* key) {}
    virtual void readBlock(uint8_t blockNumber, std::vector<uint8_t>& outData) {}
    virtual void writeBlock(uint8_t blockNumber, const std::vector<uint8_t>& data) {}
};
