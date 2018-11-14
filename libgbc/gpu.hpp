#pragma once
#include "common.hpp"
#include "tiledata.hpp"
#include <cstdint>
#include <vector>

namespace gbc
{
  class GPU
  {
  public:
    static const int SCREEN_W = 160;
    static const int SCREEN_H = 144;

    GPU(Machine&) noexcept;
    void reset() noexcept;
    void simulate();
    // the vector is resized to exactly fit the screen
    const auto& pixels() const noexcept { return m_pixels; }

    TileData create_tiledata();
    void render_scanline(int y);

    Memory&  memory() noexcept { return m_memory; }
    IO&      io() noexcept { return m_io; }

  private:
    uint32_t tile32(int x, int y);

    std::vector<uint32_t> m_pixels;
    Memory& m_memory;
    IO& m_io;
    int m_current_scanline = 0;
  };
}
