#include "gpu.hpp"

#include "machine.hpp"
#include "tiledata.hpp"
#include <cassert>

namespace gbc
{
  static const int TILES_W = GPU::SCREEN_W / TileData::TILE_W;
  static const int TILES_H = GPU::SCREEN_H / TileData::TILE_H;

  GPU::GPU(Machine& mach) noexcept
    : m_memory(mach.memory)
  {
    this->reset();
  }

  void GPU::reset() noexcept
  {

  }

  void GPU::render_to(std::vector<uint32_t>& dest)
  {
    const uint8_t scroll_y = memory().read8(IO::REG_SCY);
    const uint8_t scroll_x = memory().read8(IO::REG_SCX);

    // create tiledata object from LCDC register
    auto td = this->create_tiledata();

    dest.resize(SCREEN_W * SCREEN_H);
    for (int ty = 0; ty < TILES_H; ty++)
    for (int tx = 0; tx < TILES_W; tx++)
    {
      // get the tile id
      const int t = td.tile_id(tx, ty);
      // copy the 16-byte tile into buffer
      std::array<uint8_t, 64> buffer;
      td.pattern(t, buffer);
      // blit 8x8 tile (64 pixels) to screen
      for (int i = 0; i < 8 * 8; i++)
      {
        const int x = (scroll_x + 8 * tx + (i % 8)) % SCREEN_W;
        const int y = (scroll_y + 8 * ty + (i / 8)) % SCREEN_H;
        // convert palette to colors
        uint32_t color = 0;
        switch (buffer.at(i)) {
        case 0:
            color = 0xFFFF00FF; // magenta
            break;
        case 1:
            color = 0xFFFF0000; // red
            break;
        case 2:
            color = 0xFF00FF00; // green
            break;
        case 3:
            color = 0xFF0000FF; // blue
            break;
        }
        dest.at(y * SCREEN_W + x) = color;
      }
    } // next tile
  } // render_to(...)

  TileData GPU::create_tiledata()
  {
    const uint8_t lcdc = memory().read8(IO::REG_LCDC);
    const uint16_t ttile_base = (lcdc & 0x10) ? 0x9C00 : 0x9800;
    const uint16_t tdata_base = (lcdc & 0x08) ? 0x8000 : 0x8800;
    return TileData{memory(), ttile_base, tdata_base};
  }
}
