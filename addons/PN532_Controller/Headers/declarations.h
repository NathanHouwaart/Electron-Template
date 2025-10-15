/**
 * @file
 * @brief     Specific declarations for the nfc classes to use
 * 
 * You can alter / add content of this file to your own liking. This file contains declarations for the pn532's commands,
 * mifare commands, statuscodes etc..
 * 
 * @author    Nathan Houwaart
 * @license   See LICENSE
 */

#pragma once

#include <stdint.h>
#include <array>

namespace NFC_Controller
{
    namespace Cpp
    {
        static uint8_t ack_frame[] = { 0x00, 0x00, 0xff, 0x00, 0xff, 0x00 };
        static uint8_t ack_frame_size = 6;

        /// Mifare specific commands
        enum mifareCommands : const uint8_t{
            authenticateKeyA    = 0x60,
            authenticateKeyB    = 0x61,
            Read16Bytes         = 0x30,
            Write16Bytes        = 0xA0,
            Write4Bytes         = 0xA2,
            Incrementation      = 0xC1,
            Decrementation      = 0xC0,
            Transfare           = 0xB0,
            Restore             = 0xC2,
        };

         /// Different baudrates the for the UART protocol
        enum baudRate : const uint8_t{
            _9k6Baud   = 0x00,
            _19k2Baud  = 0x01,
            _38k4Baud  = 0x02,
            _57k6Baud  = 0x03,
            _115k2Baud = 0x04,
            _230k4Baud = 0x05,
            _460k8Baud = 0x06,
            _921k6Baud = 0x07,
            _1m2kBaud  = 0x08
        };


        /// Specific statuscodes the nfc chips can return
        enum __declspec(dllexport) statusCode : const uint8_t{
            pn532StatusOK                       = 0x00,
            pn532StatusTimeout                  = 0x01,
            pn532StatusCRCError                 = 0x02,
            pn532StatusParityError              = 0x03,
            pn532StatusCollisionError           = 0x04,
            pn532StatusMifareFramingError       = 0x05,
            pn532StatusCollisionError2          = 0x06,
            pn532StatusBufferSizeInsufficient   = 0x07,
            pn532StatusSelftestFail             = 0x08,
            pn532StatusRFBufferOverflow         = 0x09,
            pn532StatusRFFieldTimeout           = 0x0A,
            pn532StatusRFProtocolError          = 0x0B,
            pn532StatusInvalidAckFrame          = 0x0C,
            pn532StatusTemperatureError         = 0x0D,
            pn532StatusInternalBufferOverflow   = 0x0E,
            pn532StatusInvalidParameter         = 0x10,
            pn532StatusMifareAutError           = 0x14,
            pn532StatusUIDCheckByteError        = 0x23,
            pn532StatusWrongConfig              = 0x26,
            pn532StatusWrongCommand             = 0x27,
            pn532StatusReleased                 = 0x27,
            pn532StatusOverCurrent              = 0x2D,
            pn532StatusMissingDEP               = 0x2E,
            pn532statusSAMerror                 = 0x2F
        };

        /// A struct containing all 32 keys of a mifare classic 1k. Can be altered based on own card setting
        struct cardKeys{ 
                uint8_t page1A[6]  = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
                uint8_t page1B[6]  = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
                uint8_t page2A[6]  = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
                uint8_t page2B[6]  = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
                uint8_t page3A[6]  = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
                uint8_t page3B[6]  = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
                uint8_t page4A[6]  = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
                uint8_t page4B[6]  = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
                uint8_t page5A[6]  = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
                uint8_t page5B[6]  = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
                uint8_t page6A[6]  = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
                uint8_t page6B[6]  = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
                uint8_t page7A[6]  = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
                uint8_t page7B[6]  = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
                uint8_t page8A[6]  = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
                uint8_t page8B[6]  = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
                uint8_t page9A[6]  = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
                uint8_t page9B[6]  = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
                uint8_t page10A[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
                uint8_t page10B[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
                uint8_t page11A[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
                uint8_t page11B[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
                uint8_t page12A[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
                uint8_t page12B[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
                uint8_t page13A[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
                uint8_t page13B[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
                uint8_t page14A[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
                uint8_t page14B[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
                uint8_t page15A[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
                uint8_t page15B[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
                uint8_t page16A[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
                uint8_t page16B[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

                std::array<uint8_t*, 16> aKeys = {
                        page1A, page2A, page3A, page4A, page5A,
                        page6A, page7A, page8A, page9A,page10A,
                        page11A, page12A, page13A, page14A, page15A, 
                        page16A};
                std::array<uint8_t*, 16> bKeys = {
                        page1B, page2B, page3B, page4B, page5B, 
                        page6B, page7B, page8B, page9B, page10B, 
                        page11B, page12B, page13B, page14B, page15B, 
                        page16B};
        };

        namespace pn532{
            /// General commands of for the pn532
            namespace general{
                const uint8_t Pn532Address          = 0x48 >> 1;
                const uint8_t bufferSize            = 64;
                static constexpr uint8_t Ack_buffer_template_i2c[7] = { 0x01, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};  // When using i2c, the received ack frame will be one byte longer
                static constexpr uint8_t Ack_buffer_template_spi[6] = { 0x00, 0x00, 0xFF,0x00, 0xFF, 0x00};
                const uint8_t Preamble1             = 0x00;
                const uint8_t Preamble2             = 0xFF;
                const uint8_t HostToPn532           = 0xD4;
                const uint8_t Pn542ToHost           = 0xD5;
                const uint8_t DummyKey[]            = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
                const uint8_t DefaultKey[]          = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
                const uint16_t Mifare1kSize         = 1024;
                const uint8_t Mifare1kPageSize      = 16;
                const uint8_t Mifare1kUIDsize       = 4;
        
            }
    
            /// Pn532 commands. User manual p.65 & p.66
            namespace command{
                const uint8_t PerformSelftest       = 0x00;
    
                /// Different diagnose commands. User manual p.69 - p.72
                namespace diagnose {
                    const uint8_t CommunicationLineTest = 0x00;
                    const uint8_t ROMTest               = 0x01;
                    const uint8_t RAMTest               = 0x02;
                    const uint8_t PollingTestToTarget   = 0x04;
                    const uint8_t EchoBackTest          = 0x05;
                    const uint8_t CardDetectionTest     = 0x06;
                    const uint8_t AntennaTest           = 0x07;
                }
                const uint8_t GetFirmwareVersion    = 0x02;
                const uint8_t getGeneralStatus      = 0x04;
                const uint8_t readRegister          = 0x06;
                const uint8_t writeRegister         = 0x08;
                const uint8_t readGPIO              = 0x0C;
                const uint8_t writeGPIO             = 0x0E;

                /// Different GPIO related declarations
                namespace GPIO{
                    const uint8_t validationBit         = 0x80;
                    const uint8_t p32                   = 0x02;
                    const uint8_t p34                   = 0x04;           
                };
                const uint8_t setSerialBaudrate     = 0x10;
                const uint8_t SAMConfiguration      = 0x14;
        
                /// Different SAM mode configurations. User manual p.89 - p.97
                namespace SAMmode {
                    const uint8_t Normal_mode           = 0x01;
                    const uint8_t Virtual_mode          = 0x02;
                    const uint8_t Wired_card            = 0x03;
                    const uint8_t Dual_card             = 0x04;
                }
                const uint8_t RFConfiguration       = 0x32;
        
                /// Different RF configuration options. User manual p.101 - p.106
                namespace RFItem {
                    const uint8_t RFField               = 0x01;
                    const uint8_t Timings               = 0x02;
                    const uint8_t MaxRetryCommunication = 0x04;
                    const uint8_t MaxRetries            = 0x05;
                }
                const uint8_t InListPassiveTarget   = 0x4A;
       
                /// Different cardtypes the pn532 can read
                enum CardType : const uint8_t {
                    TypeA_ISO_IEC14443    = 0x00,
                    FeliCaPolling212kbps  = 0x01,
                    FeliCaPolling424kbps  = 0x02,
                    TypeB_IS0_IEC14443    = 0x03,
                    InJewelTag106kbps     = 0x04
                };

                const uint8_t InDataExchange        = 0x40;
            } // namespace command

        } // namespace pn532

    } // namespace Cpp
} // namespace NFC_Controller
