#include "Device.hpp"
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <pn532.h>
#include "PN532_Controller/Headers/SerialCommunication.h"
#include "PN532_Controller/Headers/pn532.h"

NFC_Controller::Cpp::SerialCommunication* m_protocol;
NFC_Controller::Cpp::NFC* m_nfc_chip;

bool Device::connect(const std::string& port) {
    std::cout << "Connecting to serial port: " << port << std::endl;

    char firmware_string[255];
    m_protocol->init("COM5", 115200);
    m_nfc_chip->SAMConfiguration(NFC_Controller::Cpp::pn532::command::SAMmode::Normal_mode);
    m_nfc_chip->setMaxRetries(0xFF);
    auto firmware = m_nfc_chip->getFirmwareVersion();
    if(firmware[0] == NFC_Controller::Cpp::statusCode::pn532StatusOK){
        std::cout << "Connected to NFC-reader" << std::endl;
        sprintf(firmware_string, "Found NFC device PN5%X\nFirmware version %X.%X.%X", firmware[1], firmware[2], firmware[3], firmware[4]);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    return true;
}

std::string Device::readData() {
    static int counter = 0;
    std::ostringstream ss;
    ss << "{ \"temperature\": " << (25 + (counter++ % 5)) << " }";
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return ss.str();
}