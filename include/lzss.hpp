#pragma once
#include <array>
#include <vector>
#include <span>

class LZSS{
public:
    static std::vector<uint8_t> Decode(std::span<const uint8_t> data);
    static std::vector<uint8_t> Encode(std::vector<const uint8_t> data);
};
