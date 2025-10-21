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

class MifareDesfireEV1Card : public MifareDesfireCard {
public:
    using MifareDesfireCard::MifareDesfireCard;

    virtual bool authenticateEV1(uint8_t keyNo, const std::vector<uint8_t>& aesKey) {
        (void)keyNo; (void)aesKey; return false; }

    // Retrieve true UID through explicit DESFire command (0x51)
    virtual bool getCardUID(std::vector<uint8_t>& uidOut) {
        (void)uidOut; return false; }

    virtual bool formatCard() { return false; }
    virtual bool changeKeySettings(uint8_t newSettings) { (void)newSettings; return false; }
    virtual bool commitTransaction() { return false; }
    virtual bool abortTransaction() { return false; }

    std::string prettyType() const override { return "MIFARE DESFire EV1"; }
};