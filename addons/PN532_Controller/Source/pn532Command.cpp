/**
 * @file
 * @brief     This file implements the functions declared in pn532Command.h
 * 
 * @author    Nathan Houwaart
 * @license   See LICENSE
 */

#include "..\Headers\pn532Command.h"
#include <iostream>
#include "..\Headers\hex.h"

namespace NFC_Controller
{
    namespace Cpp
    {
        setupSendCommand::setupSendCommand(const uint8_t* commandsToSend, uint8_t commandSize)
        {
            finalBuffer[0] = pn532::general::StartCode[0];
            finalBuffer[1] = pn532::general::StartCode[1];
            finalBuffer[2] = commandSize + static_cast<uint8_t>(1);
            finalBuffer[3] = calculateChecksum(finalBuffer, 2, 1);
            finalBuffer[4] = pn532::general::HostToPn532;
            uint8_t commandBufferIndex = 5;
            for (int i = 0; i < commandSize; i++)
            {
                finalBuffer[commandBufferIndex] = commandsToSend[i];
                commandBufferIndex++;
            }
            finalBuffer[commandBufferIndex] = calculateChecksum(finalBuffer, startCommand, commandSize + static_cast<uint8_t>(1));
            length = commandBufferIndex + static_cast<uint8_t>(1);
            ;
        }

        uint8_t setupSendCommand::calculateChecksum(const uint8_t* buffer, int index, uint8_t n)
        {
            uint8_t som = 0x00;
            for (uint8_t i = 0; i < n; i++)
            {
                som += buffer[index + i];
            }
            uint8_t checksum = ~som;
            return checksum + static_cast<uint8_t>(1);
        }


        // TODO ::Remove or do something with isSucces
        receivedCommand::receivedCommand(const uint8_t* receiveBufferP, uint8_t bufferSize)
        {
            isSucces = true;
            if (receiveBufferP[1] == 0x00u && receiveBufferP[2] == 0xffu)
            {
                length = receiveBufferP[3] + static_cast<uint8_t>(4);
                /* std::cout << static_cast<int>(length) << std::endl;*/
                for (uint8_t i = 0; i < length; i++)
                {
                    finalBuffer[i] = receiveBufferP[i + 3];
                }
            }
            else if (receiveBufferP[0] == 1 && receiveBufferP[1] == 0) {
                isSucces = true;
                length = receiveBufferP[4] + static_cast<uint8_t>(4);
                for (uint8_t i = 0; i < length; i++)
                {
                    finalBuffer[i] = receiveBufferP[i + 4];
                }
            }
        }

        pn532Response::pn532Response(const uint8_t* receiveBufferP, uint8_t bufferSize, uint8_t sendCommand)
        {
            std::cout << "Received buffer PN532 Response: ";
            for (int i = 0; i < bufferSize; i++)
            {
                std::cout << Hex0x(receiveBufferP[i]) << " ";
            }
            std::cout << std::endl;

            // 1. Search for the 0x00 0x00 0xFF start sequence
            size_t index = 0;
            bool foundStartSequence = false;
            
            // Search through buffer for preamble + start code (0x00 0x00 0xFF)
            for(size_t i = 0; i + 2 < bufferSize; i++)
            {
                if(receiveBufferP[i] == pn532::general::preamble &&
                   receiveBufferP[i+1] == pn532::general::StartCode[0] &&
                   receiveBufferP[i+2] == pn532::general::StartCode[1])
                {
                    index = i;
                    foundStartSequence = true;
                    std::cout << "Found start sequence at index " << index << std::endl;
                    break;
                }
            }

            if(!foundStartSequence)
            {
                std::cout << "Start sequence 0x00 0x00 0xFF not found in buffer" << std::endl;
                status = statusCode::InvalidStartCode;
                return;
            }

            // Skip past preamble and start code
            index += 3;

            // 3. Check length
            uint8_t packetLength = receiveBufferP[index++];
            if(packetLength == 0 || packetLength > sizeof(finalBuffer))
            {
                std::cout << "Invalid length" << std::endl;
                status = statusCode::InvalidLength;
                return;
            }

            // 4. Check length checksum
            uint8_t lengthChecksum = receiveBufferP[index++];
            
            if(static_cast<uint8_t>(packetLength + lengthChecksum) != 0x00)
            {
                std::cout << "Invalid length checksum" << std::endl;
                status = statusCode::InvalidLengthChecksum;
                return;
            }
         
            length = packetLength - 2;
            size_t dataStartIndex = index;

            // Check Direction byte
            uint8_t directionByte = receiveBufferP[index++];
            if(directionByte != pn532::general::Pn542ToHost)
            {
                std::cout << "Invalid direction byte" << std::endl;
                status = statusCode::InvalidDirectionByte;
                return;
            }
            
            // Check command code
            uint8_t commandCode = receiveBufferP[index++];
            if(commandCode != static_cast<uint8_t>(sendCommand + 1))
            {
                std::cout << "Invalid command code" << std::endl;
                status = statusCode::InvalidCommandCode;
                return;
            }

            // Copy data to final buffer
            for(size_t i = 0; i < length; i++)
            {
                finalBuffer[i] = receiveBufferP[index++];
            }

            std::cout << "finalbuffer data: ";
            for (uint8_t i = 0; i < length; i++)
            {
                std::cout << Hex0x(finalBuffer[i]) << " ";
            }
            std::cout << std::endl;

            // Check data checksum
            uint8_t dataChecksum = receiveBufferP[index++];
            uint8_t calculatedChecksum = calculateChecksum(receiveBufferP, dataStartIndex, packetLength);
            if(dataChecksum != calculatedChecksum)
            {
                std::cout << "Invalid data checksum: Calculated: " << Hex0x(calculatedChecksum) << " - Received: " << Hex0x(dataChecksum) << std::endl;
                status = statusCode::InvalidDataChecksum;
                return;
            }

            std::cout << "Response valid" << std::endl;
            status = statusCode::OK;
        }

        uint8_t pn532Response::calculateChecksum(const uint8_t* buffer, int index, uint8_t n)
        {
            uint8_t som = 0x00;
            for (uint8_t i = 0; i < n; i++)
            {
                som += buffer[index + i];
            }
            uint8_t checksum = ~som;
            return checksum + static_cast<uint8_t>(1);
        }

    } // namespace Cpp


} // namespace NFC_Controller
