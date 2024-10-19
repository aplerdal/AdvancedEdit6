#pragma once
#include <array>
#include <vector>
#include <span>

class LZSS{
public:
    std::vector<uint8_t> Decode(std::span<const uint8_t> data);
    std::vector<uint8_t> Encode(std::vector<const uint8_t> data);
};
