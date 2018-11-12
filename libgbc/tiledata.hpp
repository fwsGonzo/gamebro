#pragma once
#include "memory.hpp"

namespace gbc
{
  class TileData
  {
  public:
    static const int TILE_W = 8;
    static const int TILE_H = 8;

    TileData(Memory& mem, uint16_t address)
        : m_memory(mem), m_base(address) {}

    uint16_t tile_id(int tx, int ty);

    Memory& memory() noexcept { return m_memory; }

  private:
    Memory& m_memory;
    const uint16_t m_base;
  };

  inline uint16_t TileData::tile_id(int x, int y)
  {
    return memory().read8(m_base + y * 32 + x);
  }
}
