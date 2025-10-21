#pragma once

#include "../card.h"
#include <cstdint>
#include <vector>

class MifareDesfireCard : public Card {
public:
    virtual std::string prettyType() const override {
        return "MIFARE DESFire";
    }

    MifareDesfireCard(TargetInfo id)
        : Card(std::move(id)) {}

    virtual void selectApplication(uint32_t aid) {}
    virtual void readData(uint8_t fileNo, std::vector<uint8_t>& outData) {}
    virtual void writeData(uint8_t fileNo, const std::vector<uint8_t>& data) {}
};

