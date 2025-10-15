#pragma once

#include <string>
#include <iostream>

#define Log(msg) __log((msg), __FILE__, __LINE__)

inline void __log(const std::string& message, const char* file, int line) {
    std::cout << std::string("[Addon Cpp] ") + file + ":" + std::to_string(line) + " - " + message << std::endl;
}