#pragma once

#include <ostream>
#include <iomanip>

struct Hex0x
{
    uint8_t v;
    explicit Hex0x(uint8_t x) : v(x) {}
};

inline Hex0x hex0x(uint8_t x) { 
    return Hex0x{x}; 
}

inline std::ostream &operator<<(std::ostream &os, Hex0x h)
{
    const auto flags = os.flags();
    const char prev_fill = os.fill();
    os << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int(h.v) & 0xFF);
    os.flags(flags);
    os.fill(prev_fill);
    return os;
}