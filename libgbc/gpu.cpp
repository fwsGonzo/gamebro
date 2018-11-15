#include "gpu.hpp"

#include "machine.hpp"
#include "tiledata.hpp"
#include "sprite.hpp"
#include <cassert>
#include <unistd.h>

namespace gbc
{
  static const int TILES_W = GPU::SCREEN_W / TileData::TILE_W;
  static const int TILES_H = GPU::SCREEN_H / TileData::TILE_H;

  GPU::GPU(Machine& mach) noexcept
    : m_memory(mach.memory), m_io(mach.io)
  {
    this->reset();
  }

  void GPU::reset() noexcept
  {
    m_pixels.resize(SCREEN_W * SCREEN_H);

  }

  void GPU::simulate()
  {
    // nothing to do with LCD being off
    if ((io().reg(IO::REG_LCDC) & 0x80) == 0) return;

    // get current scanline
    int scanline = io().reg(IO::REG_LY);
    if (m_current_scanline != scanline && scanline < SCREEN_H)
    {
      m_current_scanline = scanline;
      //printf("Rendering scanline %u\n", scanline);
      this->render_scanline(scanline);
    }
    else {
      //printf("V-blank now %u\n", scanline);
    }
    //usleep(100);
  }

  void GPU::render_scanline(int scan_y)
  {
    const uint8_t scroll_y = memory().read8(IO::REG_SCY);
    const uint8_t scroll_x = memory().read8(IO::REG_SCX);

    // create tiledata object from LCDC register
    auto td = this->create_tiledata();

    // render whole scanline
    for (int scan_x = 0; scan_x < SCREEN_W; scan_x++)
    {
      const int sx = (scan_x + scroll_x) % 256;
      const int sy = (scan_y + scroll_y) % 256;
      const int tx = sx / TileData::TILE_W;
      const int ty = sy / TileData::TILE_H;
      // get the tile id
      const int t = td.tile_id(tx, ty);
      // copy the 16-byte tile into buffer
      const int pal = td.pattern(t, sx & 7, sy & 7);
      // convert palette to colors
      uint32_t color = 0;
      switch (pal) {
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
      m_pixels.at(scan_y * SCREEN_W + scan_x) = color;
    } // x
  } // render_to(...)

  TileData GPU::create_tiledata()
  {
    const uint8_t lcdc = memory().read8(IO::REG_LCDC);
    const auto* vram = memory().video_ram_ptr();
    const bool is_signed = (lcdc & 0x10) == 0;
    const auto* ttile_base = &vram[(lcdc & 0x10) ? 0x0 : 0x800];
    const auto* tdata_base = &vram[(lcdc & 0x08) ? 0x1C00 : 0x1800];
    return TileData{ttile_base, tdata_base, is_signed};
  }
}
