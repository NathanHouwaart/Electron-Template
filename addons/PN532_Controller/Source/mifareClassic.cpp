/**
 * @file
 * @brief     This file implements the functions declared in mifareClassic.h
 * 
 * @author    Nathan Houwaart
 * @license   See LICENSE
 */

#include <iostream>
#include <iomanip>
#include "..\Headers\mifareClassic.h"

namespace NFC_Controller
{
    namespace Cpp
    {
        void card::addPage(uint8_t* bytes, size_t bytesSize, int pageNumber)
        {
            int j = 0;
            for (size_t i = 5; i < bytesSize; i++)
            {
                cardData[(pageNumber * pageSize) + j] = bytes[i];
                j++;
            }
        }

        void card::readPage(int pageNumber) const
        {

            for (size_t i = (pageNumber * pageSize) + 0; i < ((pageNumber * pageSize) + pageSize); i++)
            {
                std::cout << "0x" << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(cardData[i]) << "  ";
            }
            std::cout << std::endl;
        }

		void card::setUID(uint8_t uid1, uint8_t uid2, uint8_t uid3, uint8_t uid4)
		{
            cardUID[0] = uid1;
            cardUID[1] = uid2;
            cardUID[2] = uid3;
            cardUID[3] = uid4;
		}

        void card::setUID(receivedCommand& response) {
            for (uint8_t i = 0; i < response.finalBuffer[9]; i++) {
                cardUID[i] = response.finalBuffer[i + 10];
            }
        }

        std::array<uint8_t, 4> card::getUID() const {
            return cardUID;
        }

        std::array<uint8_t, pn532::general::Mifare1kPageSize> card::getPage(uint8_t page) const {
            using pn532::general::Mifare1kPageSize;

            uint8_t j = 0;
            std::array<uint8_t, Mifare1kPageSize> pageData = {};
            for (int i = page * Mifare1kPageSize; i < (page * Mifare1kPageSize) + Mifare1kPageSize; i++) {
                pageData[j] = cardData[i];
                j++;
            }
            return pageData;
        }
    } // namespace Cpp
} // namespace NFC_Controller