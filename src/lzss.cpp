#include "gbalzss.hpp"
#include <vector>
#include <span>
#include <stdexcept>

inline uint32_t readLittleEndian(const std::span<const uint8_t>& data, int offset, int size) {
    uint32_t result = 0;
    for (int i = 0; i < size; ++i) {
        result |= (data[offset + i] << (i * 8));
    }
    return result;
}

std::vector<uint8_t> LZSS::Decode(std::span<const uint8_t> data)
{
    if (data.size() < 4) {
        throw std::runtime_error("Compressed data is too small.");
    }
    uint32_t decompressedSize = readLittleEndian(data, 1, 3);
    std::vector<uint8_t> decompressedData;
    decompressedData.reserve(decompressedSize);

    size_t compressedOffset = 4;
    while (decompressedData.size() < decompressedSize) {
        if (compressedOffset >= data.size()) {
            throw std::runtime_error("Malformed compressed data: out of bounds.");
        }

        // The flag byte contains 8 flags, 1 for each chunk of data
        uint8_t flagByte = data[compressedOffset++];
        for (int i = 0; i < 8 && decompressedData.size() < decompressedSize; ++i) {
            if (flagByte & 0x80) {
                // Compressed data
                if (compressedOffset + 1 >= data.size()) {
                    throw std::runtime_error("Malformed compressed data: unexpected end.");
                }

                uint8_t byte1 = data[compressedOffset++];
                uint8_t byte2 = data[compressedOffset++];

                int distance = ((byte1 & 0xF) << 8) | byte2;
                int length = (byte1 >> 4) + 3;

                if (decompressedData.size() <= distance) {
                    throw std::runtime_error("Malformed compressed data: invalid distance.");
                }

                // Copy 'length' bytes from the decompressed data
                size_t copySourceOffset = decompressedData.size() - distance - 1;
                for (int j = 0; j < length && decompressedData.size() < decompressedSize; ++j) {
                    decompressedData.push_back(decompressedData[copySourceOffset + j]);
                }
            } else {
                // Uncompressed byte
                if (compressedOffset >= data.size()) {
                    throw std::runtime_error("Malformed compressed data: unexpected end.");
                }
                decompressedData.push_back(data[compressedOffset++]);
            }
            flagByte <<= 1; // Shift the flag byte for the next chunk
        }
    }

    return decompressedData;
}

std::vector<uint8_t> Encode(std::vector<uint8_t> data)
{
    return std::vector<uint8_t> {0};
}