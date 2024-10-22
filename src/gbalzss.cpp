/*------------------------------------------------------------------------------
 * Copyright (c) 2017
 *     Michael Theall (mtheall)
 *
 * This file is part of gba-tools.
 *
 * gbalzss is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * gbalzss is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gbalzss.  If not, see <http://www.gnu.org/licenses/>.
 *----------------------------------------------------------------------------*/

// Modified for use with AE6

/** @file gbalzss.cpp
 *  @brief GBA LZSS Encoder/Decoder
 */
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <getopt.h>
#include <libgen.h>
#include <cstddef>
#include "gbalzss.hpp"

/** @brief Find last instance of a byte in a buffer
 *  @param[in] first Beginning of buffer
 *  @param[in] last  End of buffer
 *  @param[in] val   Byte to find
 *
 *  @returns iterator to found byte
 *  @retval last if no match found
 */
Buffer::const_iterator
rfind(Buffer::const_iterator first, Buffer::const_iterator last,
      const uint8_t &val)
{
  assert(last >= first);

  auto it = last;
  while(--it >= first)
  {
    if(*it == val)
      return it;
  }

  return last;
}

/** @brief Find best buffer match
 *  @param[in]  source   Source buffer
 *  @param[in]  it       Position in source buffer
 *  @param[in]  len      Maximum length to match
 *  @param[in]  max_disp Maximum displacement
 *  @param[in]  vram     VRAM-safe
 *  @param[out] outlen   Length of match
 *  @returns Iterator to best match
 *  @retval source.cend() for no match
 */
Buffer::const_iterator
find_best_match(const Buffer &source, Buffer::const_iterator it, size_t len,
                size_t max_disp, bool vram, size_t &outlen)
{
  auto begin = source.cbegin();
  auto end   = source.cend();

  assert(it > source.cbegin());
  assert(it < source.cend());

  // clamp start to maximum displacement from buffer
  if(it - begin > static_cast<ptrdiff_t>(max_disp))
    begin = it - max_disp;

  // clamp len to end of buffer
  if(end - it < static_cast<ptrdiff_t>(len))
    len = end - it;

  auto   best_start = it;
  size_t best_len = 0;

  // find nearest matching start byte
  auto last_p = it;
  auto p = rfind(begin, last_p, *it);
  while(p != last_p)
  {
    // find length of match
    size_t test_len = 1;
    for(size_t i = 1; i < len; ++i)
    {
      if(*(p+i) == *(it+i))
        ++test_len;
      else
        break;
    }

    // vram requires displacement != 1
    if(vram && (it - p) == 1)
      test_len = 0;

    if(test_len >= best_len)
    {
      // this match is the best so far, so save it
      best_start = p;
      best_len   = test_len;
    }

    // if we maximized the match, stop here
    if(best_len == len)
      break;

    // find next nearest matching byte and try again
    last_p = p;
    p = rfind(begin, last_p, *it);
  }

  if(best_len)
  {
    // we found a match, so return it
    outlen = best_len;
    return best_start;
  }

  // no match found
  outlen = 0;
  return source.cend();
}

/** @brief Output a GBA-style compression header
 *  @param[out] header Output header
 *  @param[in]  type   Compression type
 *  @param[in]  size   Uncompressed data size
 */
void
header(Buffer &buffer, uint8_t type, size_t size)
{
  buffer.push_back(type);
  buffer.push_back(size >>  0);
  buffer.push_back(size >>  8);
  buffer.push_back(size >> 16);
}

/** @brief LZ10/LZ11 compression
 *  @param[in] source Source buffer
 *  @param[in] mode   LZ mode
 *  @param[in] vram   VRAM-safe
 *  @returns Compressed buffer
 */
Buffer
lzss_encode(const Buffer &source, LZSS_t mode, bool vram)
{
  // get maximum match length
  const size_t max_len  = mode == LZ10 ? LZ10_MAX_LEN  : LZ11_MAX_LEN;

  // get maximum displacement
  const size_t max_disp = mode == LZ10 ? LZ10_MAX_DISP : LZ11_MAX_DISP;

  assert(mode == LZ10 || mode == LZ11);

  // create output buffer
  Buffer result;

  // append compression header
  header(result, mode, source.size());

  // reserve an encode byte in output buffer
  size_t code_pos = result.size();
  result.push_back(0);

  // initialize shift
  size_t shift = 8;

  // encode every byte
  auto it = source.cbegin();
  auto end = source.cend();
  while(it < end)
  {
    if(shift == 0)
    {
      // we need to encode more data, so add a new code byte
      shift = 8;
      code_pos = result.size();
      result.push_back(0);
    }

    // advance code byte bit position
    if(shift != 0)
      --shift;

    const size_t len = end - it;
    auto         tmp = source.cend();
    size_t       tmplen = 0;

    if(it == source.cbegin())
    {
      // beginning of stream must be primed with at least one value
      tmplen = 1;
    }
    else
    {
      // find best match
      tmp = find_best_match(source, it, std::min(len, max_len), max_disp, vram,
                            tmplen);
      if(tmp != source.cend())
      {
        assert(!vram || tmp - it != 1);
        assert(tmp >= source.cbegin());
        assert(tmp < it);
        assert(it - tmp <= static_cast<ptrdiff_t>(max_disp));
        assert(tmplen <= max_len);
        assert(tmplen <= len);
        assert(std::equal(it, it+tmplen, tmp));
      }
    }

    if(tmplen > 2 && tmplen < len)
    {
      // this match is long enough to be compressed; let's check if it's
      // cheaper to encode this byte as a copy and start compression at the
      // next byte
      size_t skip_len, next_len;

      // get best match starting at the next byte
      find_best_match(source, it+1, std::min(len-1, max_len), max_disp, vram,
                      skip_len);

      // check if the match is too small to compress
      if(skip_len < 3)
        skip_len = 1;

      // get best match for data following the current compressed chunk
      find_best_match(source, it+tmplen, std::min(len-tmplen, max_len),
                      max_disp, vram, next_len);

      // check if the match is too small to compress
      if(next_len < 3)
        next_len = 1;

      // if compressing this chunk and the next chunk is less valuable than
      // skipping this byte and starting compression at the next byte, mark
      // this byte as being needed to copy
      if(tmplen + next_len <= skip_len + 1)
        tmplen = 1;
    }

    if(tmplen < 3)
    {
      // this is a copy chunk; append this byte to the output buffer
      result.push_back(*it);

      // only one byte is copied
      tmplen = 1;
    }
    else if(mode == LZ10)
    {
      // mark this chunk as compressed
      assert(code_pos < result.size());
      result[code_pos] |= (1 << shift);

      // encode the displacement and length
      size_t disp = it - tmp - 1;
      assert(tmplen-3 <= 0xF);
      assert(disp <= 0xFFF);
      result.push_back(((tmplen-3) << 4) | (disp >> 8));
      result.push_back(disp);
    }
    else if(tmplen <= 0x10)
    {
      // mark this chunk as compressed
      assert(code_pos < result.size());
      result[code_pos] |= (1 << shift);

      // encode the displacement and length
      size_t disp = it - tmp - 1;
      assert(tmplen > 2);
      assert(tmplen-1 <= 0xF);
      assert(disp <= 0xFFF);
      result.push_back(((tmplen-1) << 4) | (disp >> 8));
      result.push_back(disp);
    }
    else if(tmplen <= 0x110)
    {
      // mark this chunk as compressed
      assert(code_pos < result.size());
      result[code_pos] |= (1 << shift);

      // encode the displacement and length
      size_t disp = it - tmp - 1;
      assert(tmplen >= 0x11);
      assert(tmplen-0x11 <= 0xFF);
      assert(disp <= 0xFFF);
      result.push_back((tmplen-0x11) >> 4);
      result.push_back(((tmplen-0x11) << 4) | (disp >> 8));
      result.push_back(disp);
    }
    else
    {
      // mark this chunk as compressed
      assert(code_pos < result.size());
      result[code_pos] |= (1 << shift);

      // encode the displacement and length
      size_t disp = it - tmp - 1;
      assert(tmplen >= 0x111);
      assert(tmplen-0x111 <= 0xFFFF);
      assert(disp <= 0xFFF);
      result.push_back((1 << 4) | (tmplen-0x111) >> 12);
      result.push_back(((tmplen-0x111) >> 4));
      result.push_back(((tmplen-0x111) << 4) | (disp >> 8));
      result.push_back(disp);
    }

    // advance input buffer
    it += tmplen;
  }

  // pad the output buffer to 4 bytes
  if(result.size() & 0x3)
    result.resize((result.size()+3) & ~0x3);

  // return the output data
  return result;
}

/** @brief LZ10 compression
 *  @param[in] source Source buffer
 *  @param[in] vram   VRAM-safe
 *  @returns Compressed buffer
 */
Buffer
LZSS::lz10_encode(const Buffer &source, bool vram)
{
  return lzss_encode(source, LZ10, vram);
}

/** @brief LZ11 compression
 *  @param[in] source Source buffer
 *  @param[in] vram   VRAM-safe
 *  @returns Compressed buffer
 */
Buffer
LZSS::lz11_encode(const Buffer &source, bool vram)
{
  return lzss_encode(source, LZ11, vram);
}

/** @brief LZ10 Decompression
 *  @param[in] source Source buffer
 *  @param[in] vram   VRAM-safe
 *  @returns Decompressed buffer
 */
Buffer
LZSS::lz10_decode(const Buffer &source, bool vram)
{
  if(source.size() < 4 || source[0] != LZ10)
    throw std::runtime_error("Error: Invalid LZ10 header");

  size_t size = source[1] | (source[2] << 8) | (source[3] << 16);

  bool printed_error = false;
  bool printed_vram_error = false;

  auto    src   = source.cbegin() + 4;
  uint8_t flags = 0;
  uint8_t mask  = 0;

  Buffer result;

  while(size > 0)
  {
    if(mask == 0)
    {
      // read in the flags data
      // from bit 7 to bit 0:
      //     0: raw byte
      //     1: compressed block
      flags = *src++;
      mask  = 0x80;
    }

    if(flags & mask) // compressed block
    {
      size_t len  = (((*src) & 0xF0) >> 4) + 3;
      size_t disp = ((*src++) & 0x0F) << 8;
      disp |= *src++;
      ++disp;

      if(len > size)
      {
        if(!printed_error)
        {
          std::fprintf(stderr, "Warning: Badly encoded LZ10 stream; compressed "
                       "block exceeds output length specified by header. "
                       "Truncating output.\n");
          printed_error = true;
        }

        // truncate output
        len = size;
      }

      if(result.size() < disp)
        throw std::runtime_error("Error: Badly encoded LZ10 stream; encoded "
                                 "displacement causes read prior to start of "
                                 "output buffer.");

      if(vram && !printed_vram_error)
      {
        if(disp == 1)
        {
          std::fprintf(stderr, "Warning: LZ10 stream is not vram safe.\n");
          printed_vram_error = true;
        }
      }
      
      size -= len;

      // for len, copy data from the displacement
      // to the current buffer position
      while(len-- > 0)
        result.push_back(*(std::end(result)-disp));
    }
    else // uncompressed block
    {
      // copy a raw byte from the input to the output
      result.push_back(*src++);
      --size;
    }

    mask >>= 1;
  }

  return result;
}

/** @brief LZ11 Decompression
 *  @param[in] source Source buffer
 *  @param[in] vram   VRAM-safe
 *  @returns Decompressed buffer
 */
Buffer
lz11_decode(const Buffer &source, bool vram)
{
  if(source.size() < 4 || source[0] != LZ11)
    throw std::runtime_error("Error: Invalid LZ11 header");

  size_t size = source[1] | (source[2] << 8) | (source[3] << 16);

  bool printed_error = false;
  bool printed_vram_error = false;

  auto    src   = source.cbegin() + 4;
  uint8_t flags = 0;
  uint8_t mask  = 0;

  Buffer result;

  while(size > 0)
  {
    if(mask == 0)
    {
      // read in the flags data
      // from bit 7 to bit 0:
      //     0: raw byte
      //     1: compressed block
      flags = *src++;
      mask  = 0x80;
    }

    if(flags & mask) // compressed block
    {
      size_t len;
      switch((*src) >> 4)
      {
        case 0: // extended block
          len   = (*src++) << 4;
          len  |= ((*src) >> 4);
          len  += 0x11;
          break;

        case 1: // extra extended block
          len   = ((*src++) & 0x0F) << 12;
          len  |= (*src++) << 4;
          len  |= ((*src) >> 4);
          len  += 0x111;
          break;

        default: // normal block
          len   = ((*src) >> 4) + 1;
          break;
      }

      size_t disp = ((*src++) & 0x0F) << 8;
      disp |= *src++;
      ++disp;

      if(len > size)
      {
        if(!printed_error)
        {
          std::fprintf(stderr, "Warning: Badly encoded LZ11 stream; compressed "
                       "block exceeds output length specified by header. "
                       "Truncating output.\n");
          printed_error = true;
        }

        // truncate output
        len = size;
      }

      if(result.size() < disp)
        throw std::runtime_error("Error: Badly encoded LZ11 stream; encoded "
                                 "displacement causes read prior to start of "
                                 "output buffer.");

      if(vram && !printed_vram_error)
      {
        if(disp == 1)
        {
          std::fprintf(stderr, "Warning: LZ10 stream is not vram safe.\n");
          printed_vram_error = true;
        }
      }
 
      size -= len;

      // for len, copy data from the displacement
      // to the current buffer position
      while(len-- > 0)
        result.push_back(*(std::end(result)-disp));
    }
    else // uncompressed block
    {
      // copy a raw byte from the input to the output
      result.push_back(*src++);
      --size;
    }

    mask >>= 1;
  }

  return result;
}