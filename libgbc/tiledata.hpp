#pragma once
#include "memory.hpp"

namespace gbc
{
  class TileData
  {
  public:
    static const int TILE_W = 8;
    static const int TILE_H = 8;

    TileData(const uint8_t* tile, const uint8_t* pattern, bool sign = false)
        : m_tile_base(tile), m_patt_base(pattern), m_signed(sign) {}

    int   tile_id(int tx, int ty);
    int   pattern(int t, int dx, int dy);
    void  pattern(int t, std::array<uint8_t, 64>&);
    int   pattern(const uint8_t* base, int t, int dx, int dy);

  private:
    const uint8_t* m_tile_base;
    const uint8_t* m_patt_base;
    const bool m_signed;
  };

  inline int TileData::tile_id(int x, int y)
  {
    if (m_signed == false)
        return m_tile_base[y * 32 + x];
    return (int8_t) m_tile_base[y * 32 + x];
  }

  inline int TileData::pattern(const uint8_t* base, int tid, int tx, int ty)
  {
    assert(tid >= 0 && tid < 384);
    assert(tx >= 0 && tx < 8);
    assert(ty >= 0 && ty < 8);
    const int offset = 16*tid + ty * 2;
    //printf("Offset: 16*%d + %d*2 = %d\n", tid, ty, offset);
    // get 16-bit c0, c1
    uint8_t c0 = base[offset];
    uint8_t c1 = base[offset + 1];
    // return combined 4-bits, right to left
    const int bit = 7 - tx;
    const int v0 = (c0 >> bit) & 0x1;
    const int v1 = (c1 >> bit) & 0x1;
    return v0 | (v1 << 1);
  } // pattern(...)
  inline int TileData::pattern(int tid, int tx, int ty)
  {
    return pattern(m_patt_base, tid, tx, ty);
  }

  inline void TileData::pattern(int t, std::array<uint8_t, 64>& buffer)
  {
    for (int i = 0; i < 16; i += 2)
    {
      uint8_t c0 = m_patt_base[t*16 + i];
      uint8_t c1 = m_patt_base[t*16 + i + 1];
      // read from right to left
      for (int pix = 7; pix >= 0; pix--) {
        // in each pair of c0, c1 bytes there is 8 pixels
        buffer.at(i * 4 + pix) = (c0 & 0x1) | ((c1 & 0x1) * 2);
        c0 >>= 1; c1 >>= 1;
      }
    }
  } // pattern(...)
}
