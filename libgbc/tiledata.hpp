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
    void  set_tilebase(const uint8_t* new_base) { m_tile_base = new_base; }

  private:
    const uint8_t* m_tile_base;
    const uint8_t* m_patt_base;
    const bool m_signed;
  };

  inline int TileData::tile_id(int x, int y)
  {
    if (m_signed == false)
        return m_tile_base[y * 32 + x];
    return 128 + ((int8_t*) m_tile_base)[y * 32 + x];
  }

  inline int TileData::pattern(const uint8_t* base, int tid, int tx, int ty)
  {
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
}
