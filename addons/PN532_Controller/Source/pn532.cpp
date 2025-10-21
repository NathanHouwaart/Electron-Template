/**
 * @file
 * @brief     This file implements the functions declared in pn532.h
 *
 * @author    Nathan Houwaart
 * @license   See LICENSE
 */

#include "../Headers/pn532.h"
#include "../Headers/ringbuffer.h"
#include <Windows.h>
#include <iostream>
#include <iomanip>
#include <utility>
#include <vector>
#include "../Headers/desfire.h"
#include "../Headers/hex.h"
#include "../../AddonLog.h"
#include <chrono>
#include <thread>

#include "../Headers/Commands/inListPassiveTarget.h"

namespace NFC_Controller
{
    namespace Cpp
    {
        //   for(uint8_t i = 0; i < response.length; i++){
        //         hwlib::cout << hwlib::hex << response.finalBuffer[i] << " . ";
        //     }hwlib::cout << hwlib::endl;

        CommandResult PN532_chip::executeCommand(IPn532Command &command)
        {
            // 1. Let the command describe what it needs.
            const auto request = command.buildRequest();

            // 2. Build the frame that will be sent to the PN532.
            auto frame = buildFrame(request);

            // 3. Send frame and wait for ACK.
            const auto ack = sendAndAcknowlegdeCommand(frame);
            if (ack != statusCode::pn532StatusOK)
            {
                return {pn532Response::statusCode::UnknownError, {}};
            }

            // 4. Collect the data frame (unless the command says otherwise).
            pn532Response response;
            if (command.expectsDataFrame())
            {
                constexpr uint32_t kDefaultResponseTimeoutMs = 5000;
                const uint32_t responseTimeout =
                    request.responseTimeoutMs == 0 ? kDefaultResponseTimeoutMs
                                                   : request.responseTimeoutMs;

                Result transport = get_response(static_cast<uint8_t>(request.commandCode),
                                                responseTimeout);
                if (transport.status != statusCode::pn532StatusOK)
                {
                    return {pn532Response::statusCode::UnknownError, {}};
                }
                response = std::move(transport.response);
            }
            else
            {
                response.status = pn532Response::statusCode::OK;
                response.length = 0;
            }

            // 5. Let the command parse the response.
            return command.parseResponse(response);
        }

        setupSendCommand PN532_chip::buildFrame(const CommandRequest &request)
        {
            std::vector<uint8_t> payload;
            payload.reserve(1 + request.payload.size());
            payload.push_back(request.commandCode);
            payload.insert(payload.end(), request.payload.begin(), request.payload.end());

            return setupSendCommand(payload.data(),
                                    static_cast<uint8_t>(payload.size()));
        }

        // ------------------------------------------------------------------------------- //
        // Constructor                                                                     //
        // ------------------------------------------------------------------------------- //

        PN532_chip::PN532_chip(
            communication::protocol &_protocol) : NFC(_protocol)
        {
            init();
        }

        // ------------------------------------------------------------------------------- //
        // Basic communciation functions                                                   //
        // ------------------------------------------------------------------------------- //

        void PN532_chip::init()
        {
        }

        void PN532_chip::sendData(uint8_t *commandBuffer, const uint8_t nBytes)
        {
            _protocol.wake_up();
            _protocol.send_data(commandBuffer, nBytes);
        }

        uint32_t PN532_chip::getData(uint8_t *buffer, const uint8_t nBytes)
        {
            return _protocol.receive_data(buffer, nBytes);
        }

        // TODO : response check
        statusCode PN532_chip::writeRegister(const uint16_t reg, const uint8_t val)
        {

            uint8_t adrH = (reg >> 8) & 0xFF;
            uint8_t adrL = (reg & 0xFF);

            uint8_t commands[] = {
                pn532::command::writeRegister,
                adrH,
                adrL,
                val};

            auto command = setupSendCommand(commands, sizeof(commands) / sizeof(uint8_t));

            auto result = sendAndAcknowlegdeCommand(command);
            if (result != statusCode::pn532StatusOK)
            {
                return result;
            }

            auto [status, response] = get_response(commands[0]);
            if (response.finalBuffer[3] != 0x09)
            {
                return statusCode::pn532StatusWrongCommand;
            }

            return statusCode::pn532StatusOK;
        }

        std::array<uint8_t, 2> PN532_chip::readRegister(const uint16_t reg)
        {
            std::array<uint8_t, 2> readRegister = {0};

            uint8_t adrH = (reg >> 8) & 0xFF;
            uint8_t adrL = (reg & 0xFF);

            uint8_t commands[] = {
                pn532::command::readRegister,
                adrH,
                adrL};

            auto command = setupSendCommand(commands, sizeof(commands) / sizeof(uint8_t));

            auto result = sendAndAcknowlegdeCommand(command);
            if (result != statusCode::pn532StatusOK)
            {
                return readRegister;
            }

            auto [status, response] = get_response(commands[0]);
            if (response.finalBuffer[3] != 0x07)
            {
                readRegister[0] = statusCode::pn532StatusWrongCommand;
                return readRegister;
            }

            readRegister[1] = response.finalBuffer[4];

            return readRegister;
        }

        statusCode PN532_chip::writeGPIO(uint8_t newPinState)
        {
            // make sure pin 32 and 34 are not changed
            newPinState |= (0x01 << pn532::command::GPIO::p32);
            newPinState |= (0x01 << pn532::command::GPIO::p34);

            // set validation bit
            newPinState |= pn532::command::GPIO::validationBit;

            uint8_t commands[] = {
                pn532::command::writeGPIO,
                newPinState,
                0x00 // p72 and p71 are reserved and thus not used.
            };

            auto command = setupSendCommand(commands, sizeof(commands) / sizeof(uint8_t));

            auto result = sendAndAcknowlegdeCommand(command);
            if (result != statusCode::pn532StatusOK)
            {
                return result;
            }

            auto [status, response] = get_response(commands[0]);
            if (response.finalBuffer[3] == 0x0F)
            {
                return statusCode::pn532StatusOK;
            }
            return statusCode::pn532StatusWrongCommand;
        }

        std::array<uint8_t, 2> PN532_chip::readGPIO()
        {
            std::array<uint8_t, 2> readGPIO = {0};
            uint8_t commands[] = {
                pn532::command::readGPIO};

            auto command = setupSendCommand(commands, sizeof(commands) / sizeof(uint8_t));

            auto result = sendAndAcknowlegdeCommand(command);
            if (result != statusCode::pn532StatusOK)
            {
                readGPIO[0] = result;
                return readGPIO;
            }

            auto [status, response] = get_response(commands[0]);
            if (response.finalBuffer[3] != 0x0D)
            {
                readGPIO[0] = statusCode::pn532StatusWrongCommand;
                return readGPIO;
            }

            uint8_t gpio_state = response.finalBuffer[4];
            uint8_t gpio_state_p7 = (response.finalBuffer[5] << 5);
            gpio_state |= gpio_state_p7;

            readGPIO[1] = gpio_state;

            return readGPIO;
        }

        // ------------------------------------------------------------------------------- //
        // Basic functions                                                                 //
        // ------------------------------------------------------------------------------- //

        bool PN532_chip::waitForChip(const int timeoutMs)
        {
            using namespace std::chrono;
            const auto deadline = steady_clock::now() + milliseconds(timeoutMs);
            const auto pollInterval = milliseconds(2);

            while (steady_clock::now() < deadline)
            {
                if (_protocol.data_available())
                {
                    return true; // PN532 has pushed something into the RX queue
                }
                std::this_thread::sleep_for(pollInterval);
            }
            std::cerr << "Timeout waiting for PN532 chip." << std::endl;
            return false; // timed out
        }

        bool PN532_chip::checkAck(const uint8_t *buffer, const uint8_t n)
        {
            // the ack buffers for spi and i2c slightly differ.
            if (buffer[0] != 0x00)
            {
                for (uint8_t i = 0; i < n - 1; i++)
                {
                    if (pn532::general::Ack_buffer_template_i2c[i] != buffer[i])
                    {
                        return false;
                    }
                }
                return true;
            }
            else
            {
                for (uint8_t i = 0; i < n - 2; i++)
                {
                    if (pn532::general::Ack_buffer_template_spi[i] != buffer[i])
                    {
                        return false;
                    }
                }
                return true;
            }
        }

        statusCode PN532_chip::sendAndAcknowlegdeCommand(setupSendCommand &command)
        {
            uint8_t acknowledge_buffer[6] = {};

            sendData(command.finalBuffer, command.length);
            if (!waitForChip(500))
            {
                return statusCode::pn532StatusTimeout;
            }

            std::cout << "Waiting for ACK..." << std::endl;
            getData(acknowledge_buffer, sizeof(acknowledge_buffer) / sizeof(uint8_t));

            for (int i = 0; i < 6; i++)
            {
                std::cout << "0x" << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(acknowledge_buffer[i]) << " ";
            }
            std::cout << std::endl;

            if (!checkAck(acknowledge_buffer, sizeof(acknowledge_buffer) / sizeof(uint8_t)))
            {
                return statusCode::pn532StatusInvalidAckFrame;
            }
            else
            {
                return statusCode::pn532StatusOK;
            }
        }

        Result PN532_chip::get_response(uint8_t onCommand, uint32_t timeoutMs)
        {
            if (!waitForChip(static_cast<int>(timeoutMs)))
            {
                return Result{statusCode::pn532StatusTimeout, pn532Response()};
            }

            uint8_t rawReceiveBuffer[512] = {0}; // large buffer to hold all incoming data
            uint8_t index = 0;                   // index to keep track of position in buffer
            uint32_t bytesReceived = 0;
            // Get first 4 bytes to determine length
            bytesReceived = getData(&rawReceiveBuffer[index], 4);
            if (bytesReceived < 4)
            {
                return Result{statusCode::pn532StatusTimeout, pn532Response()};
            }
            index += 4;

            // Determine length of the incoming frame
            uint8_t length = rawReceiveBuffer[3] + 3;
            getData(&rawReceiveBuffer[index], length);
            index += length;

            // Print raw receive buffer for debugging
            std::cout << "Received buffer: ";
            for (int i = 0; i < index; i++)
            {
                std::cout << "0x" << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(rawReceiveBuffer[i]) << " ";
            }
            std::cout << std::endl;

            // Construct receivedCommand object
            auto response = pn532Response(rawReceiveBuffer, index, onCommand);

            // Check if response is valid
            if (response.status != pn532Response::statusCode::OK)
            {
                return Result{statusCode::pn532StatusFrameCheckFailed, response};
            }

            return Result{statusCode::pn532StatusOK, response};
        }

        // ------------------------------------------------------------------------------- //
        // More advanced functions                                                         //
        // ------------------------------------------------------------------------------- //

        statusCode PN532_chip::performSelftest()
        {
            const uint8_t dummyData[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
            const uint8_t commands[] = {
                pn532::command::PerformSelftest,
                pn532::command::diagnose::CommunicationLineTest,
                dummyData[0], dummyData[1], dummyData[2], dummyData[3],
                dummyData[4], dummyData[5], dummyData[6]};

            auto command = setupSendCommand(commands, sizeof(commands) / sizeof(commands[0]));

            auto result = sendAndAcknowlegdeCommand(command);
            if (result != statusCode::pn532StatusOK)
            {
                return result;
            }

            auto [status, response] = get_response(commands[0]);

            for (int i = 0; i < 7; i++)
            {
                if (response.finalBuffer[i + 5] != dummyData[i])
                {
                    return statusCode::pn532StatusSelftestFail;
                }
            }

            return statusCode::pn532StatusOK;
        }

        std::array<uint8_t, 5> PN532_chip::getGeneralStatus()
        {
            std::array<uint8_t, 5> generalStatus = {0};
            uint8_t commands[] = {
                pn532::command::getGeneralStatus};
            auto command = setupSendCommand(commands, sizeof(commands) / sizeof(uint8_t));

            auto result = sendAndAcknowlegdeCommand(command);
            if (result != statusCode::pn532StatusOK)
            {
                generalStatus[0] = statusCode::pn532StatusInvalidAckFrame;
                return generalStatus;
            }

            auto [status, response] = get_response(commands[0]);
            if (status != statusCode::pn532StatusOK)
            {
                generalStatus[0] = statusCode::pn532StatusInvalidAckFrame;
                return generalStatus;
            }

            generalStatus[0] = statusCode::pn532StatusOK;
            for (int i = 1; i < 5; i++)
            {
                generalStatus[i] = response.finalBuffer[i + 3];
            }

            return generalStatus;
        }

        std::array<uint8_t, 5> PN532_chip::getFirmwareVersion()
        {
            std::array<uint8_t, 5> firmwareVersion = {0};

            uint8_t commands[] = {
                pn532::command::GetFirmwareVersion};

            auto command = setupSendCommand(commands, sizeof(commands) / sizeof(uint8_t));

            auto result = sendAndAcknowlegdeCommand(command);
            if (result != statusCode::pn532StatusOK)
            {
                firmwareVersion[0] = result;
                return firmwareVersion;
            }

            auto [status, response] = get_response(commands[0]);
            firmwareVersion[0] = status;

            if (status != statusCode::pn532StatusOK)
            {
                return firmwareVersion;
            }

            for (uint8_t i = 1; i < 5; i++)
            {
                firmwareVersion[i] = response.finalBuffer[i - 1];
            }

            return firmwareVersion;
        }

        statusCode PN532_chip::SAMConfiguration(const uint8_t mode)
        {
            uint8_t commands[] = {
                pn532::command::SAMConfiguration,
                mode};

            auto command = setupSendCommand(commands, sizeof(commands) / sizeof(uint8_t));

            auto result = sendAndAcknowlegdeCommand(command);
            auto [status, response] = get_response(commands[0]);

            // Status is set by get_response
            return status;
        }

        statusCode PN532_chip::RFField(const bool state)
        {
            uint8_t commands[] = {
                pn532::command::RFConfiguration,
                pn532::command::RFItem::RFField,
                static_cast<uint8_t>(state)};

            auto command = setupSendCommand(commands, sizeof(commands) / sizeof(uint8_t));

            auto result = sendAndAcknowlegdeCommand(command);
            if (result != statusCode::pn532StatusOK)
            {
                return result;
            }
            auto [status, response] = get_response(commands[0]);

            // Status is set by get_response
            return status;
        }

        statusCode PN532_chip::setMaxRetries(const uint8_t maxRetries)
        {
            uint8_t commands[] = {
                pn532::command::RFConfiguration,
                pn532::command::RFItem::MaxRetries,
                0xFF, // MxRtyATR    default 0xff
                0xFF, // MxRtyPSL    default 0xff
                maxRetries
                /// Source : https://www.nxp.com/docs/en/user-guide/141520.pdf
                /// P. 103   section 7.3.1
            };

            auto command = setupSendCommand(commands, sizeof(commands) / sizeof(uint8_t));

            auto result = sendAndAcknowlegdeCommand(command);
            if (result != statusCode::pn532StatusOK)
            {
                return result;
            }

            auto [status, response] = get_response(commands[0]);

            return status;
        }

        bool PN532_chip::detectCard(card &cardinfo, const uint8_t nCards, const uint8_t cardtype, Ringbuffer<uint8_t, 64> *response_p)
        {
            uint8_t commands[] = {
                pn532::command::InListPassiveTarget,
                nCards,
                cardtype};

            auto command = setupSendCommand(commands, sizeof(commands) / sizeof(uint8_t));

            auto result = sendAndAcknowlegdeCommand(command);
            if (result != statusCode::pn532StatusOK)
            {
                std::cout << "Failed to send InListPassiveTarget command" << std::endl;
                return false;
            }

            auto [status, response] = get_response(commands[0]);
            if (status != statusCode::pn532StatusOK)
            {
                std::cout << "Failed to get response for InListPassiveTarget" << std::endl;
                return false;
            }

            std::cout << "InListPassiveTarget response data: ";
            for (uint8_t i = 0; i < response.length; i++)
            {
                std::cout << Hex0x(response.finalBuffer[i]) << " ";
            }
            std::cout << std::endl;

            // response.finalBuffer now contains the parsed payload (no preamble/framing)
            // For InListPassiveTarget, the response format is:
            // [NbTg] [Tg] [SENS_RES(2)] [SEL_RES] [NFCIDLength] [NFCID] ...
            // Where NbTg = number of targets found

            if (response.length < 1)
            {
                std::cout << "Response too short" << std::endl;
                return false;
            }

            uint8_t numTargets = response.finalBuffer[0];
            std::cout << "Number of targets found: " << int(numTargets) << std::endl;

            if (numTargets == 0)
            {
                std::cout << "No card detected" << std::endl;
                return false;
            }

            // Check we have enough data for UID extraction
            // Minimal response: [NbTg][Tg][SENS_RES(2)][SEL_RES][NFCIDLen][NFCID...]
            if (response.length < 7)
            {
                std::cout << "Response data too short for UID extraction" << std::endl;
                return false;
            }

            // Extract UID (assuming ISO14443A Type A card)
            // response.finalBuffer layout:
            // [0] = NbTg (number of targets, usually 1)
            // [1] = Tg (target number, usually 1)
            // [2-3] = SENS_RES (2 bytes)
            // [4] = SEL_RES (1 byte)
            // [5] = NFCID Length (UID length)
            // [6...] = NFCID (UID bytes)

            uint8_t uidLength = response.finalBuffer[5];
            std::cout << "UID length: " << int(uidLength) << std::endl;

            if (uidLength == 4 && response.length >= 10)
            {
                // 4-byte UID
                cardinfo.setUID(response.finalBuffer[6], response.finalBuffer[7],
                                response.finalBuffer[8], response.finalBuffer[9]);
                std::cout << "Card detected with UID: "
                          << Hex0x(response.finalBuffer[6]) << " "
                          << Hex0x(response.finalBuffer[7]) << " "
                          << Hex0x(response.finalBuffer[8]) << " "
                          << Hex0x(response.finalBuffer[9]) << std::endl;
                return true;
            }
            else if (uidLength == 7 && response.length >= 13)
            {
                // 7-byte UID (some DESFire and other cards)
                std::cout << "7-byte UID detected" << std::endl;
                // For now, use first 4 bytes (you may want to extend setUID to handle 7 bytes)
                cardinfo.setUID(response.finalBuffer[6], response.finalBuffer[7],
                                response.finalBuffer[8], response.finalBuffer[9]);
                std::cout << "Card detected with UID (first 4 bytes): "
                          << Hex0x(response.finalBuffer[6]) << " "
                          << Hex0x(response.finalBuffer[7]) << " "
                          << Hex0x(response.finalBuffer[8]) << " "
                          << Hex0x(response.finalBuffer[9]) << std::endl;
                return true;
            }
            else
            {
                std::cout << "Unexpected UID length or insufficient data" << std::endl;
                return false;
            }
        }

        bool PN532_chip::detectCard(card &cardinfo, Ringbuffer<uint8_t, 64> *response_p)
        {
            receivedCommand response;
            uint32_t startindex = 0;
            uint8_t response_size = 0;
            auto response_buffer = Ringbuffer<uint8_t, 64>();

            uint8_t databuffer1[64] = {0};
            auto bytes_received = getData(databuffer1, 64);
            for (uint32_t i = 0; i < bytes_received; i++)
            {
                response_buffer.put(databuffer1[i]);
                response_p->put(databuffer1[i]);
            }
            if (response_buffer.get_size() < 2)
            {
                return false;
            }
            for (uint32_t i = 0; i < response_buffer.get_size(); i++)
            {

                if (response_buffer[i] == 0x00 && response_buffer[i + 1] == 0x00 && response_buffer[i + 2] == 0xFF)
                {
                    startindex = i + 1;
                    response_size = response_buffer[i + 3];
                }
            }

            for (int i = 0; i < response_size; i++)
            {
                databuffer1[i] = response_buffer[i + startindex];
            }

            // 1 card has been found
            if (databuffer1[startindex + 6] == 1)
            {
                cardinfo.setUID(databuffer1[startindex + 12], databuffer1[startindex + 13], databuffer1[startindex + 14], databuffer1[startindex + 15]);
                return true;
            }

            std::cout << ".";
            return false;
        }

        // Helper function to determine card type based on TargetInfo
        CardVariant PN532_chip::createCardFromTargetInfo(const TargetInfo& targetInfo)
        {
            // Determine card type based on SAK (Select Acknowledge) value
            // SAK values reference:
            // 0x08: MIFARE Classic 1K
            // 0x09: MIFARE Mini
            // 0x18: MIFARE Classic 4K
            // 0x20: MIFARE DESFire (or other ISO-DEP cards)
            // 0x28: JCOP cards
            
            const uint8_t sak = targetInfo.sak;
            
            // Check for MIFARE Classic (SAK = 0x08, 0x09, or 0x18)
            if (sak == 0x08 || sak == 0x09 || sak == 0x18) {
                Log("Detected MIFARE Classic card (SAK: 0x" + std::to_string(sak) + ")\n");
                Log("Creating MifareClassicCard\n");
                return MifareClassicCard{targetInfo};
            }
            
            // Check for DESFire or ISO-DEP compliant cards (SAK bit 5 set = 0x20)
            if ((sak & 0x20) != 0) {
                Log("Detected ISO-DEP card (SAK: 0x" + std::to_string(sak) + "), querying DESFire version...\n");
                
                // Create a temporary DESFire card with NFC reference to query version
                MifareDesfireCard tempCard{targetInfo, this};
                
                // Query the card to determine specific variant (EV1/EV2/EV3)
                uint8_t variant = tempCard.getDesfireVariant();
                
                switch (variant) {
                    case 1: // DESFire EV1
                        Log("Creating MifareDesfireEV1Card\n");
                        return MifareDesfireEV1Card{targetInfo, this};
                    
                    case 2: // DESFire EV2
                        Log("Creating MifareDesfireEV2Card\n");
                        return MifareDesfireEV2Card{targetInfo, this};
                    
                    case 3: // DESFire EV3
                        Log("Creating MifareDesfireEV3Card\n");
                        return MifareDesfireEV3Card{targetInfo, this};
                    
                    default: // Unknown or base DESFire
                        Log("Unknown DESFire variant, returning base MifareDesfireCard\n");
                        return tempCard;
                }
            }
            
            // Unknown card type
            Log("Unknown card type (SAK: 0x" + std::to_string(sak) + ")\n");
            return std::monostate{};
        }

        CardVariant PN532_chip::detectCard(TargetType targetType)
        {
            Log("Detecting card...\n");

            using Options = InListPassiveTargetCommand::Options;

            auto inListPassiveTargetCommand = InListPassiveTargetCommand(
                Options{
                    .maxTargets = 2,
                    .target = targetType});
            auto result = this->executeCommand(inListPassiveTargetCommand);
            Log("inListPassiveTargetCommand result: " + std::to_string(int(result.status)));
            
            if (result.status != pn532Response::statusCode::OK)
            {
                Log("inListPassiveTargetCommand failed with status: " + std::to_string(int(result.status)) + "\n");
                return std::monostate{};
            }

            Log("Number of detected targets: " + std::to_string(inListPassiveTargetCommand.getDetectedTargets().size()) + "\n");
            if (inListPassiveTargetCommand.getDetectedTargets().empty())
            {
                Log("No card detected.\n");
                return std::monostate{};
            }

            auto cardinfo = inListPassiveTargetCommand.getDetectedTargets();

            
            for(const auto &c : cardinfo) {
                Log("Detected card UID: ");
                for (auto byte : c.uid) {
                    std::cout << Hex0x(byte) << " ";
                }
                std::cout << std::endl;
            }

            // Determine and return the appropriate card type for the first detected card
            return createCardFromTargetInfo(cardinfo[0]);
        }

        statusCode PN532_chip::setSerialBaudrate(const baudRate br)
        {
            std::cout << "Updating Serial Baudrate" << std::endl;

            uint8_t commands[] = {
                pn532::command::setSerialBaudrate,
                br};
            auto command = setupSendCommand(commands, sizeof(commands) / sizeof(uint8_t));

            auto result = sendAndAcknowlegdeCommand(command);
            if (result != statusCode::pn532StatusOK)
            {
                return result;
            }

            auto [status, response] = get_response(commands[0]);

            sendData(ack_frame, ack_frame_size);
            Sleep(1);

            if (response.finalBuffer[3] != 0x11)
            {
                return statusCode::pn532StatusWrongCommand;
            }
            return statusCode::pn532StatusOK;
        }

        statusCode PN532_chip::initDataExchange(const uint8_t sendBuffer[], const uint8_t sendBufferSize, uint8_t receiveBuffer[], uint8_t &receiveBufferSize)
        {
            std::cout << "Initializing data exchange..." << std::endl;

            uint8_t commands[64] = {
                pn532::command::InDataExchange,
                0x01 // Card number 1
            };

            std::string logMessage = "Sendbuffer size: " + std::to_string(sendBufferSize) + "\n";
            Log(logMessage);

            for (uint8_t i = 0; i < sendBufferSize; i++)
            {
                commands[i + 2] = sendBuffer[i];
            }

            std::cout << "sendbuffer: ";
            for (uint8_t i = 0; i < sendBufferSize; i++)
            {
                std::cout << "0x" << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(sendBuffer[i]) << " ";
            }
            std::cout << std::endl;

            auto fullCommand = setupSendCommand(
                commands,
                sendBufferSize + 2);

            std::cout << "Sending data exchange command..." << std::endl;
            for (uint8_t i = 0; i < fullCommand.length; i++)
            {
                std::cout << "0x" << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(fullCommand.finalBuffer[i]) << " ";
            }
            std::cout << std::endl;

            auto result = sendAndAcknowlegdeCommand(fullCommand);
            if (result != statusCode::pn532StatusOK)
            {
                return result;
            }

            auto [status, response] = get_response(commands[0]);
            if (status != statusCode::pn532StatusOK)
            {
                return status;
            }

            if (response.finalBuffer[0] != 0x00)
            {
                return statusCode::pn532StatusInvalidResponse;
            }

            // Trim the first byte (status byte), copy rest to receiveBuffer
            receiveBufferSize = response.length - 1;
            for (uint8_t i = 0; i < receiveBufferSize; i++)
            {
                receiveBuffer[i] = response.finalBuffer[i + 1];
            }

            return statusCode::pn532StatusOK;
        }

        statusCode PN532_chip::getVersion()
        {
            auto desfire = Desfire::DesfireCard(*this);
            return desfire.getVersion();
        }

        // ------------------------------------------------------------------------------- //
        // Mifare specific functions                                                       //
        // ------------------------------------------------------------------------------- //

        statusCode PN532_chip::mifareReadPage(card &cardinfo, const uint8_t cardNumber, const uint8_t pageNumber)
        {
            uint8_t commands[] = {
                pn532::command::InDataExchange,
                cardNumber,
                mifareCommands::Read16Bytes,
                pageNumber,
            };

            auto command = setupSendCommand(
                commands,
                sizeof(commands) / sizeof(uint8_t));

            auto result = sendAndAcknowlegdeCommand(command);
            if (result != statusCode::pn532StatusOK)
            {
                return result;
            }

            auto [status, response] = get_response(commands[0]);

            if (response.finalBuffer[4] != 0x00)
            {
                std::cout << "read error: " << pageNumber << std::endl;
                return statusCode::pn532StatusMifareAutError;
            }

            cardinfo.addPage(response.finalBuffer, response.length - 2, pageNumber);

            return statusCode::pn532StatusOK;
        }

        statusCode PN532_chip::mifareAuthenticate(card &cardinfo, uint8_t cardNumber, mifareCommands AorB, uint8_t pagenr, const uint8_t *key)
        {
            std::cout << "authenticate" << std::endl;
            const std::array<uint8_t, 4> userid = cardinfo.getUID();

            uint8_t commands[] = {
                pn532::command::InDataExchange,
                cardNumber,
                AorB,
                pagenr,
                key[0], key[1], key[2], key[3], key[4], key[5],
                userid[0], userid[1], userid[2], userid[3]};

            auto command = setupSendCommand(
                commands,
                sizeof(commands) / sizeof(uint8_t));

            auto result = sendAndAcknowlegdeCommand(command);
            if (result != statusCode::pn532StatusOK)
            {
                std::cout << "authenticate not ok: " << std::hex << static_cast<int>(result) << std::endl;
                return result;
            }

            auto [status, response] = get_response(commands[0]);

            if (response.status != pn532Response::statusCode::OK)
            {
                result = statusCode::pn532StatusInternalBufferOverflow;
            }
            if (response.finalBuffer[4] != 0x00)
            {
                std::cout << "Authentication error on page: " << pagenr << std::endl;
                return statusCode::pn532StatusMifareAutError;
            }
            return statusCode::pn532StatusOK;
        }

        statusCode PN532_chip::mifareWritePage(card &cardinfo, uint8_t cardNumber, uint8_t pageNumber, const uint8_t *data)
        {
            std::cout << "Writing page: " << pageNumber << std::endl;

            uint8_t commands[] = {
                pn532::command::InDataExchange,
                cardNumber,
                mifareCommands::Write16Bytes,
                pageNumber,
                data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
                data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]};

            auto command = setupSendCommand(
                commands,
                sizeof(commands) / sizeof(uint8_t));

            auto result = sendAndAcknowlegdeCommand(command);
            if (result != statusCode::pn532StatusOK)
            {
                return result;
            }

            auto [status, response] = get_response(commands[0]);

            if (response.finalBuffer[4] != 0x00)
            {
                std::cout << "write unsuccessful" << std::endl;
                return statusCode::pn532StatusMifareFramingError;
            }

            std::cout << "write succesfull" << std::endl;
            return statusCode::pn532StatusOK;
        }

        statusCode PN532_chip::mifareReadCard(card &cardInfo, const uint8_t cardNumber, const mifareCommands AorB, const cardKeys &authenticationKeys)
        {
            std::cout << "Reading entire mifare card" << std::endl;
            uint8_t j = 0;
            for (int i = 0; i < 64; i++)
            {
                // Make sure sector is authenticated first
                if (i % 4 == 0)
                {
                    mifareAuthenticate(cardInfo, cardNumber, AorB, i + 3, authenticationKeys.aKeys[j]);
                    j++;
                }
                // Read sector pages
                mifareReadPage(cardInfo, cardNumber, i);
            }

            for (int i = 0; i < 64; i++)
            {
                if (i % 4 == 0)
                {
                    std::cout << std::endl;
                }
                cardInfo.readPage(i);
            }

            std::cout << "reading complete" << std::endl;
            std::cout << std::endl;
            std::cout << std::endl;
            std::cout << std::endl;

            return statusCode::pn532StatusOK;
        }

        statusCode PN532_chip::mifareMakeValueBlock(card &cardinfo, const uint8_t cardnumber, const mifareCommands AorB, const uint8_t pagenr, const uint8_t sector, const uint8_t *key)
        {
            std::cout << "Making value block on page: " << pagenr << std::endl;
            uint8_t dataBlockFormat[] = {0x64, 0x00, 0x00, 0x00, 0x9B, 0xFF, 0xFF, 0xFF, 0x64, 0x00, 0x00, 000, 0x01, 0xFE, 0x01, 0xFE}; // basic value block format of a mifare classic card

            auto auth_status = mifareAuthenticate(cardinfo, cardnumber, AorB, sector, key);
            auto status = mifareWritePage(cardinfo, cardnumber, pagenr, dataBlockFormat);

            if (status != statusCode::pn532StatusOK || auth_status != statusCode::pn532StatusOK)
            {
                return statusCode::pn532StatusWrongCommand;
            }

            return statusCode::pn532StatusOK;
        }

        statusCode PN532_chip::mifareIncrement(card &cardinfo, const uint8_t cardnumber, const mifareCommands AorB, const uint8_t pagenr, const uint8_t sector, const uint8_t *key, const uint32_t value)
        {

            mifareAuthenticate(cardinfo, cardnumber, AorB, sector, key);

            uint8_t commands[] = {
                pn532::command::InDataExchange,
                cardnumber,
                mifareCommands::Incrementation,
                pagenr,
                static_cast<uint8_t>(value & 0xff),
                static_cast<uint8_t>((value >> 8) & 0xff),
                static_cast<uint8_t>((value >> 16) & 0xff),
                static_cast<uint8_t>((value >> 24) & 0xff),
            };

            auto command = setupSendCommand(
                commands,
                sizeof(commands) / sizeof(uint8_t));

            auto result = sendAndAcknowlegdeCommand(command);
            if (result != statusCode::pn532StatusOK)
            {
                return result;
            }

            auto [status, response] = get_response(commands[0]);
            if (response.finalBuffer[4] != 0x00)
            {
                return statusCode::pn532StatusWrongCommand;
            }

            return statusCode::pn532StatusOK;
        }

        statusCode PN532_chip::mifareTransfer(card &cardinfo, const uint8_t cardnumber, const mifareCommands AorB, const uint8_t pagenr, const uint8_t sector, const uint8_t *key)
        {
            // make sure the block we want to transfer a value to is authenticated
            mifareAuthenticate(cardinfo, cardnumber, AorB, 0x07, key);

            uint8_t commands[] = {
                pn532::command::InDataExchange,
                cardnumber,
                mifareCommands::Transfare,
                pagenr};

            auto command = setupSendCommand(
                commands,
                sizeof(commands) / sizeof(uint8_t));

            auto result = sendAndAcknowlegdeCommand(command);
            if (result != statusCode::pn532StatusOK)
            {
                return result;
            }

            auto [status, response] = get_response(commands[0]);
            if (response.finalBuffer[4] != 0x00)
            {
                return statusCode::pn532StatusWrongCommand;
            }

            return statusCode::pn532StatusOK;
        }

        statusCode PN532_chip::mifareDecrement(card &cardinfo, const uint8_t cardnumber, const mifareCommands AorB, const uint8_t pagenr, const uint8_t sector, const uint8_t *key, const uint32_t value)
        {
            // make sure the block we want to transfer a value to is authenticated
            mifareAuthenticate(cardinfo, cardnumber, AorB, sector, key);

            uint8_t commands[] = {
                pn532::command::InDataExchange,
                cardnumber,
                mifareCommands::Decrementation,
                pagenr,
                static_cast<uint8_t>(value & 0xff),
                static_cast<uint8_t>((value >> 8) & 0xff),
                static_cast<uint8_t>((value >> 16) & 0xff),
                static_cast<uint8_t>((value >> 24) & 0xff)};

            auto command = setupSendCommand(
                commands,
                sizeof(commands) / sizeof(uint8_t));

            auto result = sendAndAcknowlegdeCommand(command);
            if (result != statusCode::pn532StatusOK)
            {
                return result;
            }

            auto [status, response] = get_response(commands[0]);
            if (response.finalBuffer[4] != 0x00)
            {
                return statusCode::pn532StatusWrongCommand;
            }

            return statusCode::pn532StatusOK;
        }
    } // namespace Cpp
} // namespace NFC_Controller
