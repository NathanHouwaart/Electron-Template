/**
 * @file
 * @brief    Implementation of the Abstract NFC interface for the pn532
 *
 * This file provides an implementation of the Abstract NFC interface class that can be used by a pn532.
 * @note    Not all commands that he pn532 provides are supported by this library, but the vast majority
 *          is implemented.
 *
 * This file contains detailled doxygen lines, but if a particulair function of thepn532 is not clear, plese
 * refer to the user manual of this chip:
 * https://www.nxp.com/docs/en/user-guide/141520.pdf
 *
 * You can also refer to the application note in order to see how the pn532 is used to read cards/ etc:
 * https://www.nxp.com/docs/en/nxp/application-notes/AN133910.pdf
 *
 * The pn532 is manufactured by NXP®. All rights reserved
 *
 * PN532 NFC RFID Module is a highly integrated transmission module for Near Field Communication at 13.56MHz. With the mode
 * switch on board, you can change easily between I2C, SPI, and UART modes. The integrated level shifter provides 3.3V or 5V
 * working voltage for your choice. In addition, it supports RFID reading and writing, and NFC function with Android phone,
 * which makes it quite convenient for wireless connection. This module is equipped with two 3mm mounting holes, of which the
 * small dimension makes it easy for using in your project! (source: http://wiki.sunfounder.cc/index.php?title=PN532_NFC_RFID_Module )
 *
 * Note: When your wiring is correct (no short circuit), the module may be a little heated, which is just normal for use.
 *
 * @author    Nathan Houwaart
 * @license   See LICENSE
 */

#pragma once

#include <stdint.h>
#include "nfc.h"
#include "pn532Command.h"
#include "command.h"

namespace NFC_Controller
{
    namespace Cpp
    {
        /// \brief
        /// Implementation of the NFC class specificly for the pn532
        class __declspec(dllexport) PN532_chip : public NFC
        {
        public:
            CommandResult executeCommand(IPn532Command &command);

            // ------------------------------------------------------------------------------- //
            // Constructor                                                                     //
            // ------------------------------------------------------------------------------- //

            /// \brief
            /// Constructor for the pn532 NFC chip
            /// \details
            /// @param _protocol    A protocol that the arduino can use to communicate with the pn532 chip
            /// @param display      A display the chip can display information on
            /// @param irq          Adress of the irq pin
            PN532_chip(
                communication::protocol &_protocol);

            // ------------------------------------------------------------------------------- //
            // Basic function(s)                                                               //
            // ------------------------------------------------------------------------------- //

            /// \brief
            /// This function initialises the pn532 chip
            /// \details
            /// The pn532 is asleep by default.
            /// By calling the init funciton the pn532 will be taken out of sleep mode
            /// Source : https://www.nxp.com/docs/en/user-guide/141520.pdf
            /// P. 99    section 7.2.11
            void init() override;

            // ------------------------------------------------------------------------------- //
            // Basic communciation functions                                                   //
            // ------------------------------------------------------------------------------- //

            /// \brief
            /// This function sends data to the pn532 over the provided interface
            /// \details
            /// When called, this funciton will send n_bytes over the provided interface for the class
            /// @param commandBuffer    Pointer to the commandBuffer we want to send
            /// @param n_bytes          Amount of bytes that needs to be send
            void sendData(uint8_t *commandBuffer, const uint8_t nBytes) override;

            /// \brief
            /// This function gets data from the pn532 over the provided interface
            /// \details
            /// When called, this funciton will get n_bytes over the provided interface for the class
            /// @param buffer       Pointer to the buffer we want to store received data in
            /// @param n_bytes      Amount of bytes that needs to be read
            uint32_t getData(uint8_t *buffer, const uint8_t nBytes) override;

            /// \brief
            /// This function is used to overwrite the content of a internal register of the pn532
            /// \details
            /// Expects a 16 bit register adress. If the register adress is only 8 bits long, the high bits of the register will be 0xFF
            /// For example : register 0xF4 will become 0xFFF4
            /// @warning May alter the behaviour of the pn532
            /// @param  reg         Internal adress of the register that needs to be written to
            /// @param  val         Value that needs to be written to the register
            /// @return statusCode  Status of the operation
            statusCode writeRegister(const uint16_t reg, const uint8_t val) override;

            /// \brief
            /// This funciton is used to read the content of a internal register of the pn532
            /// \details
            /// Expects a 16 bit register adress. If the register adress is only 8 bits long, the high bits of the register will be 0xFF
            /// For example : register 0xF4 will become 0xFFF4
            /// @param  reg         Internal adress of the register that needs to be read from
            /// @return array[0]    Statuscode of the operation
            /// @return array[1]    Value of the internal register
            std::array<uint8_t, 2> readRegister(const uint16_t reg) override;

            /// \brief
            /// The pn532 will set the output of the gpio pins according to the given new pinstate
            /// \details
            /// @warning    This function is for advanced users only. If the wrong gpio pins are written the hardware might become unstable.
            ///             a hard reset is then required to save the pn532
            ///
            ///     newPinState[0] = p30        Can be used as GPIO
            ///     newPinState[1] = p31        Can be used as GPIO
            ///     newPinState[2] = p32        Cannot be used as GPIO!
            ///     newPinState[3] = p33        Can be used as GPIO
            ///     newPinState[4] = p34        Cannot be used as GPIO!
            ///     newPinState[5] = p35        Can be used as GPIO
            ///     newPinState[6] = -          Not used
            ///     newPinState[7] = -          Not used
            ///
            /// Source : https://www.nxp.com/docs/en/user-guide/141520.pdf
            /// P. 81   section 7.2.7
            /// @return statusCode  Status of the operation
            statusCode writeGPIO(uint8_t newPinState) override;

            /// \brief
            /// The pn532 reads the value for each port and returns the information to the host controller
            /// \details
            /// Returns an 8 bit value containing the pin state for each GPIO pin of the pn532
            /// The layout of the return byte is as follows (index 0 being the LSB) :
            ///
            ///     uint8_t[0] = P30
            ///     uint8_t[1] = P31
            ///     uint8_t[2] = P32
            ///     uint8_t[3] = P33
            ///     uint8_t[4] = P34
            ///     uint8_t[5] = P35
            ///     uint8_t[6] = P71
            ///     uint8_t[7] = P72
            ///
            /// Source : https://www.nxp.com/docs/en/user-guide/141520.pdf
            /// P. 79   section 7.2.6
            /// @return array[0]    Status of the operation
            /// @return array[1]    Pin status for each GPIO pin
            std::array<uint8_t, 2> readGPIO() override;

            // ------------------------------------------------------------------------------- //
            // More basic functions                                                            //
            // ------------------------------------------------------------------------------- //

            /// \brief
            /// This function checks wether the pn532 has responded within the given timeout
            /// \details
            /// Will return false if the pn532 didnt respond in time
            /// @param timeout          Maximum time the arduino needs to wait for a response of the pn532
            /// @return true            Chip responded in time
            /// @return false           Chip didnt respond in time
            bool waitForChip(const int timeout = 2000) override;

            /// \brief
            /// Function that checks the ACKnowlege frame the pn532 sends
            /// \details
            /// This function checks wether the pn532 has acknowleged the send data to the chip
            /// The correct syntax for the ACKnowlege frame is: 0x00 0x00 0xFF 0x00 0xFF 0x00
            /// Will return false if the ACK frame has not been received
            /// @param buffer           Pointer to the acknowledge buffer we want to check
            /// @param n                Size of the ack buffer
            /// @return true            ACK == ok;
            /// @return false           ACK != ok;
            bool checkAck(const uint8_t *buffer, const uint8_t n) override;

            /// \brief
            /// Function that handles the complete communication between the pn532 and host controller
            /// \details
            /// This function will send a command to the pn532
            /// It waits till the chip responds
            /// It checks if the pn532 has send an ACKnowlege frame
            /// It waits again till the chip is ready to to send data to the host controller
            /// @param command      Command that needs to be send
            /// @param timeoutMs    Maximum time to wait for the response frame (milliseconds)
            /// @return statusCode  Status of the operation
            Result get_response(uint8_t onCommand, uint32_t timeoutMs = 20);

            /// \brief
            /// Function that handles the complete communication between the pn532 and host controller
            /// \details
            /// This function will send a command to the pn532
            /// It waits till the chip responds
            /// It checks if the pn532 has send an ACKnowlege frame
            /// It waits again till the chip is ready to to send data to the host controller
            /// @param command      Command that needs to be send
            /// @return statusCode  Status of the operation
            statusCode sendAndAcknowlegdeCommand(setupSendCommand &command);

            // ------------------------------------------------------------------------------- //
            // More advanced functions                                                         //
            // ------------------------------------------------------------------------------- //

            /// \brief
            /// This funciton gets the firmware version of the pn532
            /// \details
            /// Will return an array with the firmware version
            /// Source : https://www.nxp.com/docs/en/user-guide/141520.pdf
            /// P. 73    section 7.2.2
            /// @return std::array[0] Statuscode
            /// @return std::array[1] pn5xx version
            /// @return std::array[2] Firmware version
            /// @return std::array[3] Firmware version
            /// @return std::array[4] Support version
            std::array<uint8_t, 5> getFirmwareVersion() override;

            /// \brief
            /// This funcition lets the pn532 perform a selftest
            /// \details
            /// The communication line test is performed to test the data link between host controller and pn532
            /// Source : https://www.nxp.com/docs/en/user-guide/141520.pdf
            /// P. 69    section 7.2.1
            /// @return statusCode  status of the operation
            statusCode performSelftest() override;

            /// \brief
            /// This function allows the host controller to know the complete situation of the pn532 at a given moment
            /// \details
            /// @return std::array[0] Statuscode
            /// @return std::array[1] Last error
            /// @return std::array[2] External rf field detected (0 or 1)
            /// @return std::array[3] Amount of cards controlled
            /// @return std::array[4] SAM status
            std::array<uint8_t, 5> getGeneralStatus() override;

            /// \brief
            /// Function regulates the type of card the pn532 can communicate with
            /// \details
            /// Based on the given cardtype the pn532 needs to detect, it will adjust it's communication protocol between card and itself
            /// Furthermore, this function will store the received UID in the given card class of a card is present within the chip's rf field
            /// @param  cardinfo    Card class where the card data can be stored in
            /// @param  nCards      Amount of cards that can be detected by the pn532. (1 or 2)
            /// @param  cardtype    Type of card that needs to be read
            /// @return false       No card has been detected
            /// @return true        A card has been detected
            bool detectCard(card &cardinfo, const uint8_t nCards, const uint8_t cardtype, Ringbuffer<uint8_t, 64> *response) override;

            bool detectCard(card &cardinfo, Ringbuffer<uint8_t, 64> *response) override;

            /// \brief
            /// Metod so the pn532 can select a specific card if multiple cards are present within the RF field
            /// \details
            /// @note   This function is not yet implemented, hence it will return a statusOK
            /// @return statusCode  Status of the operation
            statusCode selectCard() override { return statusCode::pn532StatusOK; }

            /// \brief
            /// This function is used to select the data flow path by configuring the internal serial data switch
            /// \details
            /// The pn532 has te capability to be configured in four different modes:
            ///
            ///     Normal mode:    The pn532 is configured as a normal card reader
            ///     Virtual card:   The pn532 is seen as a contactless SAM card
            ///     Wired card:     The Host controller can access the SAM with standard PCD commands
            ///     Dual card:      The pn532 and SAM are configured as two seperated targets
            ///
            /// Source:  https://www.nxp.com/docs/en/user-guide/141520.pdf
            /// P. 89    section 7.2.10
            ///
            /// @param  mode        The mode we want to initialise the pn532 in
            /// @return statusCode  Status of the operation
            statusCode SAMConfiguration(const uint8_t mode) override;

            /// \brief
            /// This funcion is used to switch pn532's RF field on or off
            /// \details
            /// Source:  https://www.nxp.com/docs/en/user-guide/141520.pdf
            /// P. 101   section 7.3.1
            /// @param  state       Wether the rf field needs to be on (1) of off (0)
            /// @return statusCode  Status of the operation
            statusCode RFField(const bool state) override;

            /// \brief
            /// This function sets the max number of times the pn532 will try to activate a target in InListPassiveTarget command
            /// \details
            /// value 0x00 means only try once
            /// value 0xFF means try infinitely (default)
            /// Source:  https://www.nxp.com/docs/en/user-guide/141520.pdf
            /// P. 103   section 7.3.1
            /// @param  maxRetries  Number of times the pn532 will try to activate a target
            /// @return statusCode  Status of the operation
            statusCode setMaxRetries(const uint8_t maxRetries) override;

            /// \brief
            /// This function is used to select te baud rate on the serial link between the host controller and the pn532
            /// \details
            /// @param  br          New baud rate
            /// @return statusCode  Status of the operation
            statusCode setSerialBaudrate(const baudRate br) override;

            /// \brief
            /// This function initialises a data exchange between the pn532 and a nfc card
            /// \details
            /// @param  sendBuffer       Pointer to the buffer containing the data to send
            /// @param  sendBufferSize   Size of the send buffer
            /// @param  receiveBuffer    Pointer to the buffer to store the received data
            /// @param  receiveBufferSize Size of the receive buffer
            /// @return statusCode      Status of the operation
            statusCode initDataExchange(const uint8_t sendBuffer[], const uint8_t sendBufferSize, uint8_t receiveBuffer[], uint8_t &receiveBufferSize) override;

            /// \brief
            /// This function gets the version of the pn532 chip
            /// \details
            /// @return statusCode  Status of the operation
            statusCode getVersion() override;

            // ------------------------------------------------------------------------------- //
            // Mifare specific functions                                                       //
            // ------------------------------------------------------------------------------- //

            /// \brief
            /// Method for the pn532 to read a mifare classic card
            /// \details
            /// This method is used to read any mifare classic card (1k / 4K);
            /// @param  cardInfo    A card class where the card data can be stored in
            /// @param  cardNumber  Card that needs to be read from
            /// @param  cardNumber  Card that needs to be read
            /// @param  AorB        Wether the sector trailer blocks need to be authenticated with key A or key B
            /// @param  cardKeys    Struct to the sector trailer keys of the card
            /// @return statusCode  Status of the operation
            statusCode mifareReadCard(card &cardInfo, const uint8_t cardNumber, const mifareCommands AorB, const cardKeys &authenticationKeys) override;

            /// \brief
            /// Method for the pn532 to read a certain mifare classic page
            /// \details
            /// @param  cardinfo    A card class where the card data can be stored in
            /// @param  cardNumber  Card that needs to be read from
            /// @param  pageNumber  Pagenumber that needs to be read
            /// @return statusCode  Status of the operation
            statusCode mifareReadPage(card &cardinfo, const uint8_t cardNumber, const uint8_t pageNumber) override;

            /// \brief
            /// Method for the pn532 to write to a certain page of a mifare classic card
            /// \details
            /// @param  cardinfo    A card class where the card data can be stored in
            /// @param  cardNumber  Card that needs to be written to
            /// @param  pageNumber  Pagenumber that needs to be written to
            /// @param  data        Pointer to data array that needs to be written to the card
            /// @return statusCode  Status of the operation
            statusCode mifareWritePage(card &cardinfo, const uint8_t cardNumber, const uint8_t pageNumber, const uint8_t *data) override;

            /// \brief
            /// Method for the pn532 to authenticate a certain sector trailer block of a mifare classic card
            /// \details
            /// @param  cardinfo    A card class where the card data can be stored in
            /// @param  cardNumber  Card that needs to be written to
            /// @param  AorB        Autenticate with key a or key b
            /// @param  pageNumber  Pagenumber that needs to be authenticated
            /// @param  key         Pointer to key array that the sector trailer block needs to be autenticated with
            /// @return statusCode  Status of the operation
            statusCode mifareAuthenticate(card &cardinfo, const uint8_t cardNumber, const mifareCommands AorB, const uint8_t pagenr, const uint8_t *key) override;

            /// \brief
            /// This function will transform a given page ( pagenr ) to a valueblock.
            /// \details
            /// The balance set if the valueblock is created = 0
            /// @param  cardinfo    A card class where the card data can be stored in
            /// @param  cardNumber  Card that needs to be written to
            /// @param  AorB        Autenticate with key a or key b
            /// @param  pageNumber  Pagenumber that needs to be authenticated
            /// @param  sector      Sector where the new valueblock will be located in
            /// @param  key         Pointer to key array that the sector trailer block needs to be autenticated with
            /// @return statusCode  Status of the operation
            statusCode mifareMakeValueBlock(card &cardinfo, const uint8_t cardnumber, const mifareCommands AorB, const uint8_t pagenr, const uint8_t sector, const uint8_t *key) override;

            /// \brief
            /// This function will increment a valueblock by a given value ( value )
            /// \details
            /// @note   When this function is called, the valueblock will not be modified
            ///         only the internal temporary buffer will be updated. In order to
            ///         update the actual valueblock the transfer function needs to be
            ///         called after this function. source source: http://www.cs.ru.nl/~wouter/papers/2008-thebest-updated.pdf
            ///         p. 7    2.2 using value blocks
            ///
            /// @param  cardinfo    A card class where the card data can be stored in
            /// @param  cardNumber  Card that needs to be written to
            /// @param  AorB        Autenticate with key a or key b
            /// @param  pageNumber  Pagenumber that needs to be authenticated
            /// @param  sector      Sector where the new valueblock will be located in
            /// @param  key         Pointer to key array that the sector trailer block needs to be autenticated with
            /// @param  value       Value the sectorblock needs to be incremented by
            /// @return statusCode  Status of the operation
            statusCode mifareIncrement(card &cardinfo, const uint8_t cardnumber, const mifareCommands AorB, const uint8_t pagenr, const uint8_t sector, const uint8_t *key, const uint32_t value) override;

            /// \brief
            /// This function will decrement a valueblock by a given value ( value )
            /// \details
            /// @note   When this function is called, the valueblock will not be modified.
            ///         Only the internal temporary buffer will be updated. In order to
            ///         update the actual valueblock the transfer function needs to be
            ///         called after this function. source: http://www.cs.ru.nl/~wouter/papers/2008-thebest-updated.pdf
            ///         p. 7    2.2 using value blocks
            ///
            /// @param  cardinfo    A card class where the card data can be stored in
            /// @param  cardNumber  Card that needs to be written to
            /// @param   AorB       Autenticate with key a or key b
            /// @param  pageNumber  Pagenumber that needs to be authenticated
            /// @param  sector      Sector where the new valueblock will be located in
            /// @param  key         Pointer to key array that the sector trailer block needs to be autenticated with
            /// @param  value       Value the sectorblock needs to be decremented by
            /// @return statusCode  Status of the operation
            statusCode mifareDecrement(card &cardinfo, const uint8_t cardnumber, const mifareCommands AorB, const uint8_t pagenr, const uint8_t sector, const uint8_t *key, const uint32_t value) override;

            /// \brief
            /// This function will update a valueblock with the value that is in the internal buffer of the mifare card
            /// \brief
            /// @param cardinfo     A card class where the card data can be stored in
            /// @param cardNumber   Card that needs to be written to
            /// @param AorB         Autenticate with key a or key b
            /// @param pageNumber   Pagenumber that needs to be authenticated
            /// @param sector       Sector where the new valueblock will be located in
            /// @param key          Pointer to key array that the sector trailer block needs to be autenticated with
            /// @return statusCode  Status of the operation
            statusCode mifareTransfer(card &cardinfo, const uint8_t cardnumber, const mifareCommands AorB, const uint8_t pagenr, const uint8_t sector, const uint8_t *key) override;

        private:
            setupSendCommand buildFrame(const CommandRequest &request);
        }; // Class PN532_Chip
    } // namespace Cpp
} // namespace NFC_Controller
