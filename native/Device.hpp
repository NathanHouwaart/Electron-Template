#pragma once

#include <string>

class Device {
public:
    static bool connect(const std::string& port);
    static std::string readData();
};