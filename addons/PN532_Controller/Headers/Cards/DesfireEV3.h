#pragma once

/**
 * @file
 * @brief     Card variant class for storing card data
 * 
 * This file contains the card class that can be used to store card data read from an NFC card.
 * It provides functions to add pages, read pages and set the UID of the card.
 * @author    Nathan Houwaart
 * @license   See LICENSE
 */

#include <cstdint>
#include <vector>
#include "Desfire.h"
#include "DesfireEV2.h"

class MifareDesfireEV3Card : public MifareDesfireEV2Card {
public:
    using MifareDesfireEV2Card::MifareDesfireEV2Card;

    // EV3-specific improvements
    virtual bool enableTransactionMACChaining(bool enable = true) = 0;
    virtual bool getTransactionMACCounter(uint32_t& counter) = 0;
    virtual bool authenticateEV3(uint8_t keyNo, const std::vector<uint8_t>& key) = 0;

    std::string prettyType() const override { return "MIFARE DESFire EV3"; }
};