/**
 * @file
 * @brief     Abstract nfc class that can be implemented for any nfc reader
 * 
 * This file contains the abstract interface for any nfc reader.
 * You can inherit from this class and overwrite its abstract functions.
 * 
 * If you do so, you can use the format:
 * 
 *  nfc::NFC* nfc = &YOUR_NFC_CLASS( constructor arguments ); 
 *  nfc->getFirmwareversion();
 *  nfc->getGeneralStatus(); 
 *  
 *  And so on..
 * 
 * @author    Nathan Houwaart
 * @license   See LICENSE
 */
#pragma once


#include <stdint.h>
#include "interface.h"
#include "mifareClassic.h"
#include "ringbuffer.h"

namespace NFC_Controller
{
    namespace Cpp
    {
        /// Struct where results of functions can be stored in
        struct Result { statusCode status; receivedCommand response; };

        /// \brief
        /// Pure abstract template class that can be implemented by any nfc reader
        class __declspec(dllexport) NFC {
        protected:
            communication::protocol& _protocol;
        public:

            /// \brief
            /// Constructor for the abstract NFC class
            /// \details
            /// Takes a protocol (SPI, I2C, UART) as argument
            NFC(communication::protocol& _protocol) : _protocol(_protocol) {}

            virtual ~NFC(){}

            // ------------------------------------------------------------------------------- //   
            // Basic function(s)                                                               //
            // ------------------------------------------------------------------------------- //

            /// \brief 
            /// Abstract function to intialise a NFC card
            /// \details
            /// This method is used to initialise a NFC card
            /// For exampe: Set cerain flags, modes, etc..
            virtual void init() = 0;

            // ------------------------------------------------------------------------------- //
            // Basic communciation functions                                                   //
            // ------------------------------------------------------------------------------- //

            /// \brief
            /// Abstract function to send data to the nfc chip over the provided interface
            /// \details
            /// @note   Some nfc chips have internal firmware that handles the incoming data.
            ///         If this is not the case, the abstract function write register can be combined
            ///         with this function to achieve the same thing
            /// @param commandBuffer    Pointer to the commandBuffer we want to send
            /// @param n_bytes          Amount of bytes that needs to be send 
            virtual void sendData(uint8_t* commandBuffer, const uint8_t n_bytes) = 0;

            /// \brief
            /// Abstract function to get data from the nfc chip over the provided interface
            /// \details
            /// @note   Some nfc chips have internal firmware that handles the outgoing data.
            ///         If this is not the case, the abstract function write register can be combined
            ///         with this function to achieve the same thing
            /// @param buffer       Pointer to the buffer we want to store received data in
            /// @param n_bytes      Amount of bytes that needs to be read
            virtual uint32_t getData(uint8_t* buffer, const uint8_t n_bytes) = 0;

            /// \brief
            /// Abstract function to write a internal register of a nfc chip
            /// \details
            /// @warning May alter the behaviour of the nfc chip
            /// @param  reg         Internal adress of the register that needs to be written to
            /// @param  val         Value that needs to be written to the register
            /// @return statusCode  Status of the operation
            virtual statusCode writeRegister(const uint16_t reg, const uint8_t val) = 0;

            /// \brief
            /// Abstract function to read a internal register of a nfc chip
            /// \details
            /// @param  reg         Internal adress of the register that needs to be read from
            /// @return array[0]    Statuscode of the operation
            /// @return array[1]    Value of the internal register
            virtual std::array<uint8_t, 2> readRegister(const uint16_t reg) = 0;

            /// \brief
            /// Abstract function to write/set certain GPIO pins of the nfc chip
            /// \details
            /// @warning    This function is for advanced users only. If the wrong gpio pins are written the hardware might become unstable.
            ///             a hard reset is then required to save the nfc chip
            /// @return statusCode  Status of the operation
            virtual statusCode writeGPIO(uint8_t newPinState) = 0;

            /// \brief
            /// Abstract function to read certain GPIO pins of the nfc chip
            /// \details
            /// @return array[0]    Status of the operation   
            /// @return array[1]    Pin status for each GPIO pin
            virtual std::array<uint8_t, 2> readGPIO() = 0;

            // ------------------------------------------------------------------------------- //
            // More basic functions                                                            //
            // ------------------------------------------------------------------------------- //

            /// \brief
            /// Abstract function to check wether the nfc chip has responded within the given timeout
            /// \details
            /// @param timeout          Maximum time the host controller needs to wait for a response of the nfc chip 
            /// @return true            Chip responded in time
            /// @return false           Chip didnt respond in time
            virtual bool waitForChip(const int timeout = 2000) = 0;

            /// \brief
            /// Abstract function to check wether the send data has been acknowleged by the nfc chip
            /// \details
            /// @param buffer           Pointer to the acknowledge buffer we want to check
            /// @param n                Size of the ack buffer
            /// @return true            ACK == ok;
            /// @return false           ACK != ok;
            virtual bool checkAck(const uint8_t* buffer, const uint8_t n) = 0;

            /*/// \brief
            /// Abstract function that can handle the complete communication between the nfc chip and host controller
            /// \details
            /// Example:
            ///     This function will send a command to the nfc chip
            ///     It waits till the chip responds
            ///     It checks if the nfc chip has send an ACKnowlege frame
            ///     It waits again till the chip is ready to to send data to the host controller
            ///     Formats the received data
            /// @param command      Command that needs to be send
            /// @return Result[0]   Statuscode of the operation
            /// @return Result[1]   received command/data frm the nfc chip
            virtual Result sendCommandAndCheckAck(setupSendCommand &command) = 0;*/


            // ------------------------------------------------------------------------------- //   
            // More advanced functions                                                         //
            // ------------------------------------------------------------------------------- //

            /// \brief
            /// Abstract function to get the firmware version of the NFC chip
            /// \details
            /// @return array[0] Status of the operation
            /// @return array[1] Device version
            /// @return array[2] Firmware verion
            /// @return array[3] Firmware version
            /// @return array[4] Suppor version
            virtual std::array<uint8_t, 5> getFirmwareVersion() = 0;

            /// \brief
            /// Abstract function to let the nfc chip perform a selftest
            /// \details
            /// @return statusCode Status of the operation
            virtual statusCode performSelftest() = 0;

            /// \brief
            /// Abstract function to get the current status of a nfc chip 
            /// \details
            /// @return std::array woth general status
            virtual std::array<uint8_t, 5> getGeneralStatus() = 0;

            /// \brief
            /// Abstract function for a nfc chip to detect wether a nfc card is within it´s rf field
            /// \details
            ///
            /// @warning    not all nfc chips natively implement anticollision. Be carefull when nCards is set to
            ///             2 or more cards. This may cause unwanted behaviour if not implemented correctly
            ///
            /// @param  cardinfo    Card class where the card data can be stored in
            /// @param  nCards      Amount of cards that can be detected by the nfc card
            /// @param  cardtype    Type of card that needs to be read
            /// @return false       No card has been detected
            /// @return true        A card has been detected
            virtual bool detectCard(card& cardinfo, const uint8_t nCards, const uint8_t cardtype, Ringbuffer<uint8_t, 64>* response) = 0;


            virtual bool detectCard(card& cardinfo, Ringbuffer<uint8_t, 64>* response) = 0;

            /// \brief
            /// Abstract function to select / or deselect a specific card that is in the nfc's rf field.
            /// \details
            /// This can be used when multiple cards are within the rf field of a NFC chip
            /// @return statusCode Status of the operation
            virtual statusCode selectCard() = 0;

            /// \brief
            /// Abstract function to set the mode of the internal SAM (Security Access Module)
            /// \details
            /// Not all chips come with a SAM. Read the documentation of the specific chip thoroughly
            /// To know wether to implement it or not
            ///
            /// @param  mode        The mode we want to initialise the nfc chip in
            /// @return statusCode  Status of the operation
            virtual statusCode SAMConfiguration(const uint8_t mode) = 0;

            /// \brief
            /// Abstract function to switch nfc's RF field on or off
            /// \details
            /// @param  state       Wether the rf field needs to be on (1) of off (0)
            /// @return statusCode  Status of the operation
            virtual statusCode RFField(const bool state) = 0;

            /// \brief
            /// 
            /// \details
            /// @param  maxRetries  number of times the pn532 will try to activate a target
            /// @return statusCode  status of the operation
            virtual statusCode setMaxRetries(const uint8_t maxRetries) = 0;

            /// \brief
            /// Abstract function to change the serial baudrate beteween a nfc chip and host controller
            /// \details
            /// @param  br          Baudrate the serial communication needs to be changed to
            /// @return statuscode  Status of the operation
            virtual statusCode setSerialBaudrate(const baudRate br) = 0;


            // ------------------------------------------------------------------------------- //
            // Mifare specific functions                                                       //
            // ------------------------------------------------------------------------------- //

            /// \brief
            /// Method to read a entire mifare classic card
            /// \details
            /// @param cardinfo     a card class where the card data can be stored in
            /// @param cardNumber   card that needs to be read
            /// @param AorB         wether the sector trailer blocks need to be authenticated with key A or key B
            /// @param cardKeys     struct to the sector trailer keys of the card
            /// @return statusCode  status of the operation
            virtual statusCode mifareReadCard(card& cardInfo, const uint8_t cardNumber, const mifareCommands AorB, const cardKeys& authenticationKeys) = 0;

            /// \brief
            /// Method to read a certain mifare classic page
            /// \details
            /// @param cardinfo     a card class where the card data can be stored in
            /// @param cardNumber   card that needs to be read from
            /// @param pageNumber   pagenumber that needs to be read
            /// @return statusCode  status of the operation
            virtual statusCode mifareReadPage(card& cardinfo, const uint8_t cardNumber, const uint8_t pageNumber) = 0;

            /// \brief
            /// Method to write to a certain page of a mifare classic card
            /// \details
            /// @param cardinfo     a card class where the card data can be stored in
            /// @param cardNumber   card that needs to be written to
            /// @param pageNumber   pagenumber that needs to be written to
            /// @param data         pointer to data array that needs to be written to the card
            /// @return statusCode  status of the operation
            virtual statusCode mifareWritePage(card& cardinfo, const uint8_t cardNumber, const uint8_t pageNumber, const uint8_t* data) = 0;

            /// \brief
            /// Method to write to authenticate a certain sector trailer block of a mifare classic card
            /// \details
            /// @param cardinfo     a card class where the card data can be stored in
            /// @param cardNumber   card that needs to be written to
            /// @param AorB         autenticate with key a or key b
            /// @param pageNumber   pagenumber that needs to be authenticated
            /// @param key          pointer to key array that the sector trailer block needs to be autenticated with
            /// @return statusCode  status of the operation
            virtual statusCode mifareAuthenticate(card& cardinfo, const uint8_t cardNumber, const mifareCommands AorB, const uint8_t pagenr, const uint8_t* key) = 0;

            /// \brief
            /// Abstract function to transform a given page ( pagenr ) to a valueblock.
            /// \details
            /// @param  cardinfo    A card class where the card data can be stored in
            /// @param  cardNumber  Card that needs to be written to
            /// @param  AorB        Autenticate with key a or key b
            /// @param  pageNumber  Pagenumber that needs to be authenticated
            /// @param  sector      Sector where the new valueblock will be located in
            /// @param  key         Pointer to key array that the sector trailer block needs to be autenticated with
            /// @return statusCode  Status of the operation
            virtual statusCode mifareMakeValueBlock(card& cardinfo, const uint8_t cardnumber, const mifareCommands AorB, const uint8_t pagenr, const uint8_t sector, const uint8_t* key) = 0;

            /// \brief
            /// Abstract function to increment a valueblock by a given value ( value ) 
            /// \details
            /// @note   When this function is implemented, the valueblock will not be modified
            ///         only the internal temporary buffer will be updated. In order to 
            ///         update the actual valueblock the transfer function needs to be
            ///         called after this function. source source: http://www.cs.ru.nl/~wouter/papers/2008-thebest-updated.pdf 
            ///         p. 7  -  2.2 using value blocks
            ///
            /// @param  cardinfo    A card class where the card data can be stored in
            /// @param  cardNumber  Card that needs to be written to
            /// @param  AorB        Autenticate with key a or key b
            /// @param  pageNumber  Pagenumber that needs to be authenticated
            /// @param  sector      Sector where the new valueblock will be located in
            /// @param  key         Pointer to key array that the sector trailer block needs to be autenticated with
            /// @param  value       Value the sectorblock needs to be incremented by
            /// @return statusCode  Status of the operation
            virtual statusCode mifareIncrement(card& cardinfo, const uint8_t cardnumber, const mifareCommands AorB, const uint8_t pagenr, const uint8_t sector, const uint8_t* key, const uint32_t value) = 0;

            /// \brief
            /// Abstract function to decrement a valueblock by a given value ( value ) 
            /// \details
            /// @note   When this function is implemented, the actual valueblock of the card will not be modified.
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
            virtual statusCode mifareDecrement(card& cardinfo, const uint8_t cardnumber, const mifareCommands AorB, const uint8_t pagenr, const uint8_t sector, const uint8_t* key, const uint32_t value) = 0;

            /// \brief
            /// Abstract function to update a valueblock with the value that is in the internal buffer of the mifare card
            /// \brief
            /// @param cardinfo     A card class where the card data can be stored in
            /// @param cardNumber   Card that needs to be written to
            /// @param AorB         Autenticate with key a or key b
            /// @param pageNumber   Pagenumber that needs to be authenticated
            /// @param sector       Sector where the new valueblock will be located in
            /// @param key          Pointer to key array that the sector trailer block needs to be autenticated with
            /// @return statusCode  Status of the operation
            virtual statusCode mifareTransfer(card& cardinfo, const uint8_t cardnumber, const mifareCommands AorB, const uint8_t pagenr, const uint8_t sector, const uint8_t* key) = 0;
        };
    } // namespace Cpp
} // namespace NFC_Controller
