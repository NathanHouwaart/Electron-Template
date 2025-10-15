/**
 * @file
 * @brief    Data object for a nfc card where data can be stored in
 * 
 * This file provides a data object where a nfc reader can store the read cardinformation in. For now,
 * this card is fixed for just a Mifare classic 1k card. In the future templates will be added
 * so that the user can initialise a card class for any card.
 * 
 * TODO: Add template
 * 
 * @author    Nathan Houwaart
 * @license   See LICENSE
 */

#pragma once

#include "pn532Command.h"

namespace NFC_Controller
{
    namespace Cpp
    {

        /// \brief
        /// card class
        /// \details
        /// This class saves the data of the card in a buffer
        /// You can either read, add orr add a UID.

        // template<uint8_t UIDsize, int cardSize>
        class __declspec(dllexport) card {
        private:
            size_t                                                      pageSize;
            std::array<uint8_t, pn532::general::Mifare1kUIDsize>   cardUID = { 0 };
            uint8_t cardData[pn532::general::Mifare1kSize] = { 0 }; // initialise the array with 0's
        public:
            /// \brief
            /// Default onstructor for a normal nfc card 
            /// \details
            /// A normal nfc card has a page size of 16 bytes, hence this default constructor sets the page size to 16.
            card() :pageSize(pn532::general::Mifare1kPageSize) {};

            /// \brief
            /// Add a page to the card data buffer
            /// \details
            /// This command stores the received bytes from the pn532 chip in a buffer.
            /// This way we can read the whole card faster.
            /// @param  bytes       Pointer to an array of the bytes that need to be stored in the card object
            /// @param  bytesSize   Size of the array with data
            /// @param  pageNumber  Page the data needs to be stored in
            void addPage(uint8_t* bytes, size_t bytesSize, int pageNumber);

            /// \brief
            /// Read a page from the card buffer
            /// \details
            /// This command reads a specific page form the card buffer and prints it out in the terminal
            /// @param  pagenumber      Number of the page needs to be read
            void readPage(int pageNumber) const;

            /// \brief
            /// This function sets the UID of a card object
            /// \details
            /// @param uid  Pointer to an array of the UID that needs to be stored in the card data type
            void setUID(uint8_t uid1, uint8_t uid2, uint8_t uid3, uint8_t uid4);

            /// \brief
            /// This function sets the UID of a card object
            /// \details
            /// This function takes a receivedCommand as argument and will abduct the currect UID from it
            /// @param  receivedCommand     a received command that the UID needs to be abducted from
            void setUID(receivedCommand& response);

            /// \brief 
            /// This function will return the stored UID of a card data object
            /// \details
            /// @return array   User ID that is stored in the data object
            std::array<uint8_t, pn532::general::Mifare1kUIDsize> getUID() const;

            /// \brief
            /// This function will return the data of one specific page that is stored in this data object
            /// \details
            /// @return array   Content of one particulair page
            std::array<uint8_t, pn532::general::Mifare1kPageSize> getPage(uint8_t page) const;
        }; // class card
    } // namespace Cpp
} // namespace NFC_Controler