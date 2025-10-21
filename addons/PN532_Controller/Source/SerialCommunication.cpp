#include "..\Headers\SerialCommunication.h"
#include <iostream>
#include <iomanip>

namespace NFC_Controller
{
	namespace Cpp
	{
		SerialCommunication::SerialCommunication()
		{
		}

		bool SerialCommunication::init(std::string portname, uint32_t baudrate)
		{
			// Open the given port
			if (!open_port(portname))
			{
				return false;
			}

			// Set COM port timeout settings
			if (!set_timeout())
			{
				return false;
			}

			// Set COM port baudrate
			if (!set_baudrate(baudrate))
			{
				return false;
			}

			// Confirm that the port is open
			return true;
		}

		bool SerialCommunication::close_port()
		{
			if (is_open)
			{
				auto res = CloseHandle(serial_handler);
				if (res)
				{
					is_open = false;
				}
				return res;
			}
			return false;
		}

		void SerialCommunication::wake_up()
		{
			uint8_t data[10] = {};
			this->send_data(data, 10);
		}

		bool SerialCommunication::open_port(std::string portname)
		{
			auto name = "\\\\.\\" + portname;

			serial_handler = CreateFileA(
				name.c_str(),
				GENERIC_READ | GENERIC_WRITE, 0, NULL,
				OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

			if (serial_handler == INVALID_HANDLE_VALUE)
			{
				fprintf(stderr, "error setting up comm port\n");
				return false;
			}
			is_open = true;
			return true;
		}

		bool SerialCommunication::send_data(uint8_t *data, const uint8_t n_bytes)
		{
			DWORD iBytesWritten = 0;
			return WriteFile(serial_handler, data, n_bytes, &iBytesWritten, NULL);
		}

		bool SerialCommunication::send_data(const uint8_t *data, const uint8_t n_bytes)
		{
			DWORD iBytesWritten = 0;
			return WriteFile(serial_handler, data, n_bytes, &iBytesWritten, NULL);
		}

		uint32_t SerialCommunication::receive_data(uint8_t *receive_buffer, const uint8_t n_bytes)
		{
			DWORD dwBytesTransferred = 0;

			BOOL success = ReadFile(serial_handler, receive_buffer, n_bytes, &dwBytesTransferred, 0);

			if (!success)
			{
				DWORD error = GetLastError();
				std::cerr << "ReadFile failed with error: " << error << std::endl;
				// You might want to close the port or set a flag indicating the connection is broken
				return 0;
			}

			std::cout << "=== SERIAL COMMUNICATION ===" << std::endl;
			std::cout << "SERIAL RECEVING: " << int(n_bytes) << " BYTES" << std::endl;
			std::cout << "ACTUAL BYTES RECEIVED: " << int(dwBytesTransferred) << std::endl;
			std::cout << "SERIAL COM RECEIVED DATA: ";
			for (uint32_t i = 0; i < dwBytesTransferred; i++)
			{
				std::cout << "0x" << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(receive_buffer[i]) << " ";
			}
			std::cout << std::endl;
			std::cout << "============================" << std::endl;
			return dwBytesTransferred;
		}

		bool SerialCommunication::set_timeout()
		{
			// Set COM port timeout settings
			serial_timeouts.ReadIntervalTimeout = 50;
			serial_timeouts.ReadTotalTimeoutConstant = 50;
			serial_timeouts.ReadTotalTimeoutMultiplier = 1;
			serial_timeouts.WriteTotalTimeoutConstant = 50;
			serial_timeouts.WriteTotalTimeoutMultiplier = 10;

			if (SetCommTimeouts(serial_handler, &serial_timeouts) == 0)
			{
				fprintf(stderr, "Error setting timeouts\n");
				CloseHandle(serial_handler);
				return 0;
			}
			return 1;
		}

		bool SerialCommunication::set_baudrate(uint32_t baudrate)
		{
			dcb_handler.DCBlength = sizeof(dcb_handler);
			if (GetCommState(serial_handler, &dcb_handler) == 0)
			{
				fprintf(stderr, "Error getting device state\n");
				CloseHandle(serial_handler);
				return 1;
			}

			dcb_handler.BaudRate = baudrate;
			dcb_handler.ByteSize = 8;
			dcb_handler.StopBits = ONESTOPBIT;
			dcb_handler.Parity = NOPARITY;
			if (SetCommState(serial_handler, &dcb_handler) == 0)
			{
				fprintf(stderr, "Error setting device parameters\n");
				CloseHandle(serial_handler);
				return 0;
			}
			return 1;
		}

		bool SerialCommunication::data_available() const
		{
			DWORD errors;
			COMSTAT status;

			if (ClearCommError(serial_handler, &errors, &status) == 0)
			{
				std::cerr << "ClearCommError failed." << std::endl;
				return false;
			}

			return status.cbInQue > 0;
		}
	} // namespace Cpp
} // namespace NFC_Controller
