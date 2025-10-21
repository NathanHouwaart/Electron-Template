#pragma once

/**
 * @file
 * @brief     Card variant class for storing card data
 * 
 * This file contains the card class that can be used to store card data read from an NFC card.
 * It provides functions to add pages, read pages and set the UID of the card.
 * @author    Nathan Houwaart
 * 
 * @license   See LICENSE
 */

#include <cstdint>
#include <variant>
#include <array>

#include "Cards/Desfire.h"
#include "Cards/DesfireEV1.h"
#include "Cards/DesfireEV2.h"
#include "Cards/DesfireEV3.h"

namespace NFC_Controller
{
    namespace Cpp
    {
        /// \brief
        /// cardVariant class
        /// \details
        /// This class is a variant that can hold different types of card objects.
        /// It can hold a MifareDesfireCard, MifareDesfireEV1Card, MifareDesfireEV2Card, or MifareDesfireEV3Card.
        using CardVariant = std::variant<
            std::monostate,
            MifareDesfireCard,
            MifareDesfireEV1Card
            // MifareDesfireEV2Card,
            // MifareDesfireEV3Card
        >;
    }
}