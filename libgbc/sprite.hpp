#pragma once
#include "memory.hpp"

namespace gbc
{
  class Sprite
  {
  public:
    static const int SPRITE_W = 8;
    static const int SPRITE_H = 8;

    Sprite() {}
    void draw(int idx, Memory&, std::vector<uint32_t>&);

    bool hidden() const noexcept {
      return ypos == 0 || ypos >= 160 || xpos == 0 || xpos >= 168;
    }

    bool above() const noexcept {
      return attr & 0x80;
    }
    bool flipx() const noexcept {
      return attr & 0x20;
    }
    bool flipy() const noexcept {
      return attr & 0x40;
    }
    int pal() const noexcept {
      return (attr & 0x10) >> 4;
    }
    int cgb_bank() const noexcept {
      return (attr & 0x8) >> 3;
    }
    int cgb_pal() const noexcept {
      return attr & 0x3;
    }

    int x() const noexcept { return xpos - 8; }
    int y() const noexcept { return ypos - 16; }

    bool is_within_scanline(int scan_y) const noexcept {
      return scan_y >= y() && scan_y < y() + SPRITE_H;
    }

  private:
    uint8_t ypos;
    uint8_t xpos;
    uint8_t pattern;
    uint8_t attr;
  };

  inline void Sprite::draw(int i, Memory& mem, std::vector<uint32_t>& buffer)
  {
    uint8_t pattern = mem.read8(0x8000 + i);
    
  }
}
