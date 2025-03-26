#pragma once

#include <iostream>
#include <random>
#include <sstream>
#include <iomanip>

namespace baldwin
{

inline std::string generateUUID()
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis(0, 0xFFFFFFFFFFFFFFFF);

    uint64_t part1 = dis(gen);
    uint64_t part2 = dis(gen);

    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    ss << std::setw(8) << (part1 >> 32) << "-";
    ss << std::setw(4) << ((part1 >> 16) & 0xFFFF) << "-";
    ss << std::setw(4) << (part1 & 0xFFFF) << "-";
    ss << std::setw(4) << ((part2 >> 48) & 0xFFFF) << "-";
    ss << std::setw(12) << (part2 & 0xFFFFFFFFFFFF);

    return ss.str();
}

}; // namespace baldwin
