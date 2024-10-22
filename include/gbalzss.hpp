#pragma once

#include <vector>
#include <span>
#include <stdint.h>

/** @brief LZSS maximum encodable size */
#define LZSS_MAX_ENCODE_LEN 0x00FFFFFF

/** @brief LZSS maximum (theoretical) decodable size
 *  (LZSS_MAX_ENCODE_LEN+1)*9/8 + 4 - 1
 */
#define LZSS_MAX_DECODE_LEN 0x01B00003

/** @brief LZ10 maximum match length */
#define LZ10_MAX_LEN  18

/** @brief LZ10 maximum displacement */
#define LZ10_MAX_DISP 4096

/** @brief LZ11 maximum match length */
#define LZ11_MAX_LEN  65808

/** @brief LZ11 maximum displacement */
#define LZ11_MAX_DISP 4096

/** @brief LZ compression mode */
enum LZSS_t
{
  LZ10 = 0x10, ///< LZ10 compression
  LZ11 = 0x11, ///< LZ11 compression
};

/** @brief Buffer object */
typedef std::span<uint8_t> Buffer;

class LZSS {
public:
    static std::vector<uint8_t> lz10_encode(const Buffer &source, bool vram);
    static std::vector<uint8_t> lz11_encode(const Buffer &source, bool vram);
    static std::vector<uint8_t> lz10_decode(const Buffer &source, bool vram);
    static std::vector<uint8_t> lz11_decode(const Buffer &source, bool vram);
private:
};