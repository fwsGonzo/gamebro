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
    this->m_video_offset = 0;
  }

  void GPU::simulate()
  {
    // nothing to do with LCD being off
    if ((io().reg(IO::REG_LCDC) & 0x80) == 0) return;

    auto& vblank   = io().vblank;
    auto& lcd_stat = io().lcd_stat;
    auto& reg_stat = io().reg(IO::REG_STAT);

    const uint64_t t = io().machine().now();
    const uint64_t period = t - vblank.last_time;

    // vblank always when screen on
    static const int SCANLINE_CYCLES = 456;
    if (t >= vblank.last_time + SCANLINE_CYCLES)
    {
      vblank.last_time = t;
      // scanline LY increment logic
      static const int MAX_LINES = 154;
      this->m_ly = (this->m_ly + 1) % MAX_LINES;
      io().reg(IO::REG_LY) = this->m_ly;

      if (this->m_ly == 0) {
        // start of new frame
      }
      else if (this->m_ly == 144)
      {
        assert(this->is_vblank());
        // vblank interrupt
        io().trigger(vblank);
        // modify stat
        reg_stat &= 0xfc;
        reg_stat |= 0x1;
        // if STAT vblank interrupt is enabled
        if (reg_stat & 0x10) io().trigger(lcd_stat);
      }
    }
    // STAT coincidence bit
    if (this->m_ly == io().reg(IO::REG_LYC)) {
      // STAT interrupt (if enabled)
      if ((reg_stat & 0x4) == 0
        && reg_stat & 0x40) io().trigger(lcd_stat);
      setflag(true, reg_stat, 0x4);
    }
    else {
      setflag(false, reg_stat, 0x4);
    }
    m_current_mode = reg_stat & 0x3;
    // STAT mode & scanline period modulation
    if (!this->is_vblank())
    {
      if (m_current_mode < 2 && period < 80+172)
      {
        // enable MODE 2: OAM search
        // check if OAM interrupt enabled
        if (reg_stat & 0x20) io().trigger(lcd_stat);
        reg_stat &= 0xfc;
        reg_stat |= 0x2;
      }
      else if (m_current_mode == 2 && period >= 80)
      {
        // enable MODE 3: Scanline VRAM
        reg_stat &= 0xfc;
        reg_stat |= 0x3;
      }
      else if (m_current_mode == 3 && period >= 80+172)
      {
        // enable MODE 0: H-blank
        if (reg_stat & 0x8) io().trigger(lcd_stat);
        reg_stat &= 0xfc;
        reg_stat |= 0x0;
      }
      //printf("Current mode: %u -> %u period %lu\n",
      //        current_mode(), reg_stat & 0x3, period);
      m_current_mode = reg_stat & 0x3;
    }

    // MODE 3: Scanline VRAM
    if (m_current_mode == 3)
    {
      // get current scanline
      const int scanline = this->m_ly;
      if (m_current_scanline != scanline && scanline < SCREEN_H)
      {
        m_current_scanline = scanline;
        this->render_scanline(m_current_scanline);
      }
    }
  }

  bool GPU::is_vblank() const noexcept {
    return m_ly >= 144;
  }
  bool GPU::is_hblank() const noexcept {
    return m_current_mode == 3;
  }

  void GPU::render_and_vblank()
  {
    for (int y = 0; y < SCREEN_H; y++) {
      this->render_scanline(y);
    }
    // call vblank handler directly
    io().vblank.callback(io().machine(), io().vblank);
  }

  void GPU::render_scanline(int scan_y)
  {
    const uint8_t scroll_y = memory().read8(IO::REG_SCY);
    const uint8_t scroll_x = memory().read8(IO::REG_SCX);
    const uint8_t pal = memory().read8(IO::REG_BGP);

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
      const int idx = td.pattern(t, sx & 7, sy & 7);
      const uint32_t color = this->colorize(pal, idx);
      m_pixels.at(scan_y * SCREEN_W + scan_x) = color;
    } // x
  } // render_to(...)

  uint32_t GPU::colorize(const uint8_t pal, const uint8_t idx)
  {
    const uint8_t color = (pal >> (idx*2)) & 0x3;
    // no conversion
    if (m_pixelmode == PM_PALETTE) return color;
    // convert palette to colors
    switch (color) {
    case 0:
        return 0xFFFFFFFF; // white
    case 1:
        return 0xFFA0A0A0; // light-gray
    case 2:
        return 0xFF777777; // gray
    case 3:
        return 0xFF000000; // black
    }
    return 0xFFFF00FF; // magenta = invalid
  }

  uint16_t GPU::bg_tiles() {
    const uint8_t lcdc = memory().read8(IO::REG_LCDC);
    return (lcdc & 0x08) ? 0x9C00 : 0x9800;
  }
  uint16_t GPU::tile_data() {
    const uint8_t lcdc = memory().read8(IO::REG_LCDC);
    return (lcdc & 0x10) ? 0x8000 : 0x8800;
  }

  TileData GPU::create_tiledata()
  {
    const uint8_t lcdc = memory().read8(IO::REG_LCDC);
    const bool is_signed = (lcdc & 0x10) == 0;
    const auto* vram = memory().video_ram_ptr();
    //printf("Background tiles: 0x%04x  Tile data: 0x%04x\n",
    //        bg_tiles(), tile_data());
    const auto* ttile_base = &vram[bg_tiles() - 0x8000];
    const auto* tdata_base = &vram[tile_data() - 0x8000];
    return TileData{ttile_base, tdata_base, is_signed};
  }

  std::vector<uint32_t> GPU::dump_background()
  {
    std::vector<uint32_t> data(256 * 256);
    const uint8_t pal = memory().read8(IO::REG_BGP);
    // create tiledata object from LCDC register
    auto td = this->create_tiledata();
    const auto* vram = memory().video_ram_ptr();

    for (int y = 0; y < 256; y++)
    for (int x = 0; x < 256; x++)
    {
      // get the tile id
      const int t = td.tile_id(x >> 3, y >> 3);
      // copy the 16-byte tile into buffer
      const int idx = td.pattern(t, x & 7, y & 7);
      data.at(y * 256 + x) = this->colorize(pal, idx);
    }
    return data;
  }
  std::vector<uint32_t> GPU::dump_tiles()
  {
    std::vector<uint32_t> data(16*24 * 8*8);
    const uint8_t pal = memory().read8(IO::REG_BGP);
    // create tiledata object from LCDC register
    auto td = this->create_tiledata();

    for (int y = 0; y < 24*8; y++)
    for (int x = 0; x < 16*8; x++)
    {
      int tile = (y / 8) * 16 + (x / 8);
      // copy the 16-byte tile into buffer
      const int idx = td.pattern(tile, x & 7, y & 7);
      data.at(y * 128 + x) = this->colorize(pal, idx);
    }
    return data;
  }

  void GPU::set_video_bank(uint8_t bank)
  {
    this->m_video_offset = bank * 0x2000;
  }
}
