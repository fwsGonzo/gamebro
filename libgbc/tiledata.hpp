#pragma once
#include "memory.hpp"

namespace gbc
{
  class TileData
  {
  public:
    static const int TILE_W = 8;
    static const int TILE_H = 8;

    TileData(Memory& mem, uint16_t tiles, uint64_t patterns)
        : m_memory(mem), m_tile_base(tiles), m_patt_base(patterns) {}

    uint16_t tile_id(int tx, int ty);
    void     pattern(int t, std::array<uint8_t, 64>&);

    Memory& memory() noexcept { return m_memory; }

  private:
    Memory& m_memory;
    const uint16_t m_tile_base;
    const uint16_t m_patt_base;
  };

  inline uint16_t TileData::tile_id(int x, int y)
  {
    return memory().read8(m_tile_base + y * 32 + x);
  }

  inline void TileData::pattern(int t, std::array<uint8_t, 64>& buffer)
  {
    for (int i = 0; i < 16; i += 2)
    {
      uint8_t c0 = memory().read8(m_patt_base + t*16 + i);
      uint8_t c1 = memory().read8(m_patt_base + t*16 + i + 1);
      for (int pix = 0; pix < 8; pix++) {
        // in each pair of c0, c1 bytes there is 8 pixels
        // read from right to left
        buffer.at(i * 4 + 7 - pix) = (c0 & 0x1) | ((c1 & 0x1) * 2);
        c0 >>= 1; c1 >>= 1;
      }
    }
  } // pattern(...)
}
