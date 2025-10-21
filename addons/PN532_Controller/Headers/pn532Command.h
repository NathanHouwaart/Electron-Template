/**
 * @file
 * @brief    This file contains two data objects that handle and trim the outgoing and received commands of a pn532
 * 
 * This file provides data objects that help standardize the received pn532 commands to a fixed format.
 * Normally, the pn532 sends a frame to the host controller with information in response to a command send. However,
 * depending on what interface is used, the frame might look a little different.
 * 
 * A general pn532 frame looks like:
 * 
 *  0x00, 0x00, 0xFF, LEN, LCS, TFI, PD0, PD1, PDn, DCS, 0x00
 * 
 * Where:
 *  - 0x00, 0x00, 0xFF  = preamble (start of a command)
 *  - LEN               = Length of the receiven command
 *  - LCS               = Checksum of lenght
 *  - TFI               = Frame identifier ( D4: host -> pn532. D5: pn542 -> host)
 *  - PD0, PD1, PDn     = Packed data
 *  - DCS               = Packed data Checksum
 *  - 0x00              = Postamble (end of a command)
 * 
 * For more detailled information, please refer to the user manual:
 * https://www.nxp.com/docs/en/user-guide/141520.pdf    P. 28  - 6.2.1.1
 * 
 * @author    Nathan Houwaart
 * @license   See LICENSE
 */

#pragma once


#include "declarations.h"
#include <stdint.h>
#include <iostream>
#include <iomanip>
#include "hex.h"

namespace NFC_Controller
{
    namespace Cpp
    {

        /// \brief
        /// setupSendCommand
        /// \details
        /// This is a data object that manages te setup of a send command for the pn532
        /// A commandBuffer can be up to 64 bytes long
        /// The appropriate constructors and operators are provided.
        class __declspec(dllexport) setupSendCommand {
        public:
            uint8_t length;
            uint8_t startCommand = 4;
            uint8_t finalBuffer[64];

            /// \brief
            /// Constructor for setupSendCommand
            /// \details
            /// This constructor manages the proper layout for a command that can be send to the pn532.
            /// It will add preambles, calculate the checksums, and calculate the proper commandsize.
            setupSendCommand(const uint8_t* commandsToSend, uint8_t commandSize);

            /// \brief
            /// Function to calculate the checksum of a command.
            /// \details
            /// The pn532 will use the 
            /// This function will calculate the cecksum by adding all commands to each other.
            /// The checksum will be inverted and 1 will be added for the correct checksum
            uint8_t calculateChecksum(const uint8_t* buffer, int index, uint8_t n);
        };

        /// \brief
        /// ReceiveCommand
        /// \details
        /// This data object manages the received command from a pn532. 
        /// The command buffer can be up to 64 bytes long.
        /// The appropriate constructors and operators are provided
        class __declspec(dllexport) receivedCommand {
        public:
            // todo: implement error here, and implement the first frame bytes extraction so
            // that direction and error bytes can be extracted properly
            uint8_t         length;
            bool            isSucces;
            uint8_t         finalBuffer[64];        // trimmed buffer containing received data plus some info

            /// \brief
            /// Default constructor for a received command
            /// \details
            /// Since no command has been received, isSucces will be false
            receivedCommand() :length(0), isSucces(false) {};

            /// \brief
            /// Constructor for receivedCommand
            /// \details
            /// This constructor manages the proper layout for a command that the pn532 has send to the host controller.
            /// It will check wether the command has been succesfully received.
            /// It will remove preambles and trim the command to the correct size
            receivedCommand(const uint8_t* receiveBufferP, uint8_t bufferSize);
        };

        // 0x00 0x00 0xff 0x15 0xeb 
        // 0xd5 0x4b 0x01 0x01 0x03 0x44 0x20 0x07 0x04 0x1a 0x56 0x1a 0x99 0x5b 0x80 0x06 0x75 0x77 0x81 0x02 0x80 0x79 0x00 0x00
        class pn532Response {
        public:
            enum class statusCode : uint8_t {
                OK,
                InvalidPreamble,
                InvalidStartCode,
                InvalidLength,
                InvalidLengthChecksum,
                InvalidDataChecksum,
                InvalidDirectionByte,
                InvalidCommandCode,
                UnknownError
            };

            statusCode      status;
            uint8_t         length;
            uint8_t         finalBuffer[64];

            pn532Response() :length(0), status(statusCode::UnknownError) {};

            pn532Response(const uint8_t* receiveBufferP, uint8_t bufferSize, uint8_t expectedCommandCode);

            void printInfo() const {
                std::cout << "PN532 Response Info:" << std::endl;
                std::cout << "Status: " << static_cast<int>(status) << std::endl;
                std::cout << "Length: " << static_cast<int>(length) << std::endl;
                std::cout << "Final Buffer: ";
                for (uint8_t i = 0; i < length; i++) {
                    std::cout << Hex0x(finalBuffer[i]) << " ";
                }
                std::cout << std::dec << std::endl;
            }

        private:
            uint8_t calculateChecksum(const uint8_t* buffer, int index, uint8_t n);
        };

    } //namespace Cpp
} // namespace NFC_Controller
