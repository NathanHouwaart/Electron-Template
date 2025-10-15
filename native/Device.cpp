#include "Device.hpp"
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>

bool Device::connect(const std::string& port) {
    std::cout << "Connecting to serial port: " << port << std::endl;
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