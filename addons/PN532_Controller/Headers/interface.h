/**
 * @file
 * @brief     Abstract protocol class and it's specific protocol implementation
 * 
 * @author    Nathan Houwaart
 * @license   See LICENSE
 */

#pragma once

#include "pn532Command.h"
#include <stdint.h>

namespace communication {

    /// \brief
    /// Abstract protocol class
    class __declspec(dllexport) protocol
    {

    public:
        virtual ~protocol(){}

        /// \brief
        /// Abstract function to wakeup / initialise a protocol
        virtual void wake_up() = 0;

        /// \brief
        /// Abstract function to send data over the given protocol
        /// \details
        /// @param      commandBuffer   Pointer to the start of the commandbuffer that needs to be send
        /// @param      nBytes          Amount of bytes that need to send 
        virtual bool send_data(uint8_t* commandBuffer, uint8_t nBytes) = 0;

        /// \brief
        /// Abstract function to send data over the given protocol
        /// \details
        /// @param      commandBuffer   Pointer to the start of the commandbuffer that needs to be send
        /// @param      nBytes          Amount of bytes that need to send 
        virtual bool send_data(const uint8_t* commandBuffer, const uint8_t nBytes) = 0;


        /// \brief
        /// Abstract function to receive data over the given protocol
        /// \details
        /// @param      receiveBuffer   Pointer to the start of the receiveBuffer where data can be stored in
        /// @param      nBytes          Amount of bytes that need to be received over the given protocol
        virtual uint32_t receive_data(uint8_t* receiveBuffer, uint8_t nBytes) = 0;

        /// \brief
       /// Abstract function to update the baudrate of the given protocol
       /// \details
       /// @param      baidrate   new baudrate of the protocol
        virtual bool set_baudrate(uint32_t baudrate) = 0;

        /// \brief
        /// Abstract function to check if data is available to read
        /// \details
        /// @return true     Data is available to read
        /// @return false    No data is available to read
        virtual bool data_available() const = 0;
    };

}
