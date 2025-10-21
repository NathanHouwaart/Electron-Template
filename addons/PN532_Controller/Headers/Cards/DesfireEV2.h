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
#include "DesfireEV1.h"

class MifareDesfireEV2Card : public MifareDesfireEV1Card {
public:
    using MifareDesfireEV1Card::MifareDesfireEV1Card;

    // EV2 new features
    virtual bool authenticateEV2(uint8_t keyNo, const std::vector<uint8_t>& key) = 0;
    virtual bool createBackupFile(uint8_t fileNo, uint16_t size) = 0;
    virtual bool getKeyVersion(uint8_t keyNo, uint8_t& version) = 0;

    // Transaction MAC (integrity)
    virtual bool enableTransactionMAC(bool enable = true) = 0;
    virtual bool setVirtualCardID(const std::vector<uint8_t>& vcid) = 0;

    std::string prettyType() const override { return "MIFARE DESFire EV2"; }
};