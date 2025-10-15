/**
 * @file SerialCommunication.hpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2021-01-21
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#pragma once

#include "interface.h"
#include <Windows.h>
#include <cstdint>
#include <string>
#include <array>

namespace NFC_Controller
{
    namespace Cpp
    {

        class __declspec(dllexport) SerialCommunication : public communication::protocol {
        public:

            /**
             * @brief Construct a new Serial Communication object
             *
             */
            SerialCommunication();

            bool init(std::string portname, uint32_t baudrate);

            /**
             * @brief Closes serial connection
             *
             */
            bool close_port();

            /**
             * @brief Opens Serial Connection
             *
             */
            bool open_port(std::string portname);

            /**
             * @brief Wake
             *
             */
            void wake_up() override;

            /**
             * @brief SendData
             *
             */
            bool send_data(uint8_t* commandBuffer, const uint8_t n_bytes) override;

            /**
             * @brief SendData
             *
             */
            bool send_data(const uint8_t* commandBuffer, const uint8_t n_bytes) override;

            /**
             * @brief Receive data
             *
             */
            uint32_t receive_data(uint8_t* commandBuffer, const uint8_t n_bytes) override;

            /**
             * @brief
             *
             */
            bool set_timeout();

            /**
             * @brief function to set new baudrate of the serial communication
             * @param baudrate new baudrate
             */
            bool set_baudrate(uint32_t baudrate);

        private:
            bool            is_open = false;
            HANDLE          serial_handler = { 0 };
            DCB             dcb_handler = { 0 };
            COMMTIMEOUTS    serial_timeouts = { 0 };
        };
    } // namespace Cpp
} // namespace NFC_Controller

