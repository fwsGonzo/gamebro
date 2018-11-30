#pragma once
#include "common.hpp"
#include "tiledata.hpp"
#include "sprite.hpp"
#include <cstdint>
#include <vector>

namespace gbc
{
  enum pixelmode_t {
    PM_RGBA = 0,     // regular 32-bit RGBA
    PM_PALETTE = 1,  // no conversion
  };
  class GPU
  {
  public:
    static const int SCREEN_W = 160;
    static const int SCREEN_H = 144;

    GPU(Machine&) noexcept;
    void reset() noexcept;
    void simulate();
    void set_pixelmode(pixelmode_t);
    // the vector is resized to exactly fit the screen
    const auto& pixels() const noexcept { return m_pixels; }
    // trap on palette changes
    using palchange_func_t = delegate<void(uint8_t idx, uint16_t clr)>;
    void on_palchange(palchange_func_t func) { m_on_palchange = func; }
    // get default GB palette
    static std::array<uint32_t, 4> default_gb_colors() noexcept;

    void render_and_vblank();
    bool is_vblank() const noexcept;
    bool is_hblank() const noexcept;

    void    set_mode(uint8_t mode);
    uint8_t get_mode() const noexcept;

    uint16_t video_offset() const noexcept { return m_video_offset; }
    bool     video_writable() noexcept;
    void     set_video_bank(uint8_t bank);
    void     lcd_power_changed(bool state);

    bool lcd_enabled() const noexcept;
    bool window_enabled() const noexcept;
    std::pair<int, int> window_size();
    bool window_visible();
    int  window_x();
    int  window_y();

    // CGB palette registers
    enum pal_t { PAL_BG, PAL_SPR };
    uint8_t& getpal(uint16_t index);
    void     setpal(uint16_t index, uint8_t value);

    Machine& machine() noexcept { return m_memory.machine(); }
    Memory&  memory() noexcept { return m_memory; }
    IO&      io() noexcept { return m_io; }
    std::vector<uint32_t> dump_background();
    std::vector<uint32_t> dump_window();
    std::vector<uint32_t> dump_tiles();

  private:
    uint64_t scanline_cycles();
    uint64_t oam_cycles();
    uint64_t vram_cycles();
    void render_scanline(int y);
    void do_ly_comparison();
    TileData create_tiledata(uint16_t tiles, uint16_t patt);
    sprite_config_t sprite_config();
    std::vector<const Sprite*> find_sprites(const sprite_config_t&);
    uint32_t colorize_tile(int tattr, uint8_t idx);
    uint32_t colorize_sprite(const Sprite*, sprite_config_t&, uint8_t);
    uint16_t get_cgb_color(size_t idx) const;
    uint32_t expand_cgb_color(uint16_t c16) const noexcept;
    uint32_t expand_dmg_color(uint8_t idx) const noexcept;
    // addresses
    uint16_t bg_tiles() const noexcept;
    uint16_t window_tiles() const noexcept;
    uint16_t tile_data() const noexcept;

    Memory& m_memory;
    IO& m_io;
    uint8_t&    m_reg_lcdc;
    uint8_t&    m_reg_stat;
    uint8_t&    m_reg_ly;
    std::vector<uint32_t> m_pixels;
    palchange_func_t m_on_palchange = nullptr;
    pixelmode_t m_pixelmode = PM_RGBA;
    int m_current_scanline = 0;
    uint16_t m_video_offset = 0x0;

    // 0-63: tiles 64-127: sprites
    std::array<uint8_t, 128> m_cgb_palette;
  };

  inline void GPU::set_pixelmode(pixelmode_t pm) {
    this->m_pixelmode = pm;
  }

  inline std::array<uint32_t, 4> GPU::default_gb_colors() noexcept
  {
    #define mRGB(r, g, b) (r | (g << 8) | (b << 16) | (255u << 24))
    return std::array<uint32_t, 4>{
      mRGB(175, 203,  70), // least green
      mRGB(121, 170, 109), // less green
      mRGB( 34, 111,  95), // very green
      mRGB(  8,  41,  85)  // dark green
    };
    #undef mRGB
  }
}
