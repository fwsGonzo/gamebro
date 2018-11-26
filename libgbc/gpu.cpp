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
    : m_memory(mach.memory), m_io(mach.io),
      m_reg_lcdc {io().reg(IO::REG_LCDC)},
      m_reg_stat {io().reg(IO::REG_STAT)},
      m_reg_ly   {io().reg(IO::REG_LY)}
  {
    this->reset();
  }

  void GPU::reset() noexcept
  {
    m_pixels.resize(SCREEN_W * SCREEN_H);
    this->m_video_offset = 0;
  }
  uint64_t GPU::scanline_cycles()
  {
    return memory().speed_factor() * 456;
  }
  void GPU::simulate()
  {
    static const int OAM_CYCLES      = 80;
    static const int VRAM_CYCLES     = 172;

    // nothing to do with LCD being off
    if (!this->lcd_enabled()) {
      return;
    }

    auto& vblank   = io().vblank;
    auto& lcd_stat = io().lcd_stat;

    const uint64_t t = machine().now();
    const uint64_t period = t - lcd_stat.last_time;
    bool new_scanline = false;

    // scanline logic when screen on
    if (t >= lcd_stat.last_time + scanline_cycles())
    {
      lcd_stat.last_time += scanline_cycles();
      // scanline LY increment logic
      static const int MAX_LINES = 154;
      m_current_scanline = (m_current_scanline + 1) % MAX_LINES;
      m_reg_ly = m_current_scanline;
      new_scanline = true;
      //printf("LY is now %#x\n", this->m_current_scanline);

      if (m_reg_ly == 144)
      {
        assert(this->is_vblank());
        // MODE 1: vblank interrupt
        io().trigger(vblank);
        // modify stat
        this->set_mode(1);
        // if STAT vblank interrupt is enabled
        if (m_reg_stat & 0x10) io().trigger(lcd_stat);
      }
      else if (m_reg_ly == 153)
      {
        // BUG: LY stays at 0 for extra long
        m_reg_ly = 0;
      }
      // LY == LYC comparison on each line
      this->do_ly_comparison();
    }
    // STAT mode & scanline period modulation
    if (!this->is_vblank())
    {
      if (get_mode() < 2 && new_scanline)
      {
        // enable MODE 2: OAM search
        // check if OAM interrupt enabled
        if (m_reg_stat & 0x20) io().trigger(lcd_stat);
        set_mode(2);
      }
      else if (get_mode() == 2 && period >= OAM_CYCLES)
      {
        // enable MODE 3: Scanline VRAM
        set_mode(3);
        // render a scanline
        this->render_scanline(m_current_scanline);
        // TODO: perform HDMA transfers here!
      }
      else if (get_mode() == 3 && period >= OAM_CYCLES+VRAM_CYCLES)
      {
        // enable MODE 0: H-blank
        if (m_reg_stat & 0x8) io().trigger(lcd_stat);
        set_mode(0);
      }
      //printf("Current mode: %u -> %u period %lu\n",
      //        current_mode(), m_reg_stat & 0x3, period);
    }
  }

  bool GPU::is_vblank() const noexcept {
    return m_current_scanline >= 144;
  }
  bool GPU::is_hblank() const noexcept {
    return get_mode() == 3;
  }
  uint8_t GPU::get_mode() const noexcept {
    return m_reg_stat & 0x3;
  }
  void GPU::set_mode(uint8_t mode)
  {
    this->m_reg_stat &= 0xfc;
    this->m_reg_stat |= mode & 0x3;
  }

  void GPU::do_ly_comparison()
  {
    const bool equal = m_reg_ly == io().reg(IO::REG_LYC);
    // STAT coincidence bit
    setflag(equal, m_reg_stat, 0x4);
    // STAT interrupt (if enabled) when LY == LYC
    if (equal && (m_reg_stat & 0x40)) io().trigger(io().lcd_stat);
  }

  void GPU::render_and_vblank()
  {
    for (int y = 0; y < SCREEN_H; y++) {
      this->render_scanline(y);
    }
    // call vblank handler directly
    io().vblank.callback(machine(), io().vblank);
  }

  void GPU::render_scanline(int scan_y)
  {
    const uint8_t scroll_y = memory().read8(IO::REG_SCY);
    const uint8_t scroll_x = memory().read8(IO::REG_SCX);
    const uint8_t pal  = memory().read8(IO::REG_BGP);

    // create tiledata object from LCDC register
    auto td = this->create_tiledata(bg_tiles(), tile_data());
    // window visibility
    const bool window = this->window_visible() && scan_y >= window_y();
    auto wtd = this->create_tiledata(window_tiles(), tile_data());

    // create sprite configuration structure
    auto sprconf = this->sprite_config();
    sprconf.scan_y = scan_y;
    // create list of sprites that are on this scanline
    auto sprites = this->find_sprites(sprconf);

    // render whole scanline
    for (int scan_x = 0; scan_x < SCREEN_W; scan_x++)
    {
      const int sx = (scan_x + scroll_x) % 256;
      const int sy = (scan_y + scroll_y) % 256;
      // get the tile id
      const int t = td.tile_id(sx / 8, sy / 8);
      // copy the 16-byte tile into buffer
      const int tidx = td.pattern(t, sx & 7, sy & 7);
      uint32_t color = this->colorize(pal, tidx);
      // window on can be under sprites
      if (window && scan_x >= window_x()-7)
      {
        const int sx = scan_x - window_x()+8;
        const int sy = scan_y - window_y();
        // draw window pixel
        const int t = wtd.tile_id(sx / 8, sy / 8);
        const int tidx = wtd.pattern(t, sx & 7, sy & 7);
        color = this->colorize(pal, tidx);
      }

      // render sprites within this x
      sprconf.scan_x = scan_x;
      for (const auto* sprite : sprites) {
        const uint8_t idx = sprite->pixel(sprconf);
        if (idx != 0) {
          if (!sprite->behind() || tidx == 0) {
            color = this->colorize(sprconf.palette[sprite->pal()], idx);
          }
        }
      }
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

  bool GPU::lcd_enabled() const noexcept {
    return m_reg_lcdc & 0x80;
  }
  bool GPU::window_enabled() const noexcept {
    return m_reg_lcdc & 0x20;
  }
  bool GPU::window_visible() {
    return window_enabled() && window_x() < 166 && window_y() < 143;
  }
  int GPU::window_x()
  {
    return io().reg(IO::REG_WX);
  }
  int GPU::window_y()
  {
    return io().reg(IO::REG_WY);
  }

  uint16_t GPU::bg_tiles() const noexcept {
    return (m_reg_lcdc & 0x08) ? 0x9C00 : 0x9800;
  }
  uint16_t GPU::window_tiles() const noexcept {
    return (m_reg_lcdc & 0x40) ? 0x9C00 : 0x9800;
  }
  uint16_t GPU::tile_data() const noexcept {
    return (m_reg_lcdc & 0x10) ? 0x8000 : 0x8800;
  }

  TileData GPU::create_tiledata(uint16_t tiles, uint16_t patterns)
  {
    const uint8_t lcdc = memory().read8(IO::REG_LCDC);
    const bool is_signed = (lcdc & 0x10) == 0;
    const auto* vram = memory().video_ram_ptr();
    //printf("Background tiles: 0x%04x  Tile data: 0x%04x\n",
    //        bg_tiles(), tile_data());
    const auto* tile_base = &vram[tiles    - 0x8000];
    const auto* patt_base = &vram[patterns - 0x8000];
    return TileData{tile_base, patt_base, is_signed};
  }
  sprite_config_t GPU::sprite_config()
  {
    const uint8_t lcdc = memory().read8(IO::REG_LCDC);
    sprite_config_t config;
    config.patterns = memory().video_ram_ptr();
    config.palette[0] = memory().read8(IO::REG_OBP0);
    config.palette[1] = memory().read8(IO::REG_OBP1);
    config.scan_x = 0;
    config.scan_y = 0;
    config.mode8x16 = lcdc & 0x4;
    return config;
  }

  std::vector<const Sprite*> GPU::find_sprites(const sprite_config_t& config)
  {
    const auto* oam = memory().oam_ram_ptr();
    const Sprite* sprite = (Sprite*) oam;
    const Sprite* sprite_end = sprite + 40;
    std::vector<const Sprite*> results;

    while (sprite < sprite_end) {
      if (sprite->hidden() == false)
      if (sprite->is_within_scanline(config)) results.push_back(sprite);
      sprite++;
    }
    return results;
  }

  std::vector<uint32_t> GPU::dump_background()
  {
    std::vector<uint32_t> data(256 * 256);
    const uint8_t pal = memory().read8(IO::REG_BGP);
    // create tiledata object from LCDC register
    auto td = this->create_tiledata(bg_tiles(), tile_data());

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
    // tiles start at the beginning of video RAM
    auto td = this->create_tiledata(0x8000, tile_data());

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
  bool GPU::video_writable() noexcept
  {
    if (lcd_enabled()) {
      return is_vblank() || is_hblank();
    }
    return true; // when LCD is off, always writable
  }
  void GPU::lcd_power_changed(const bool online)
  {
    printf("Screen turned %s\n", online ? "ON" : "OFF");
    auto& lcd_stat = io().lcd_stat;
    if (online)
    {
      // at the start of a new frame
      lcd_stat.last_time = machine().now() - scanline_cycles();
      m_current_scanline = 153;
      m_reg_ly = m_current_scanline;
    }
    else
    {
      // LCD off, just reset to LY 0
      m_current_scanline = 0;
      m_reg_ly = 0;
      // modify stat to V-blank?
      this->set_mode(1);
    }
  }
}
