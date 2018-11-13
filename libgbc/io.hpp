#pragma once
#include <array>
#include <cstdint>
#include "common.hpp"
#include "interrupt.hpp"

namespace gbc
{
  class IO
  {
  public:
    enum regnames_t {
      REG_P1    = 0xff00,
      // TIMER
      REG_DIV   = 0xff04,
      REG_TIMA  = 0xff05,
      REG_TMA   = 0xff06,
      REG_TAC   = 0xff07,
      // SOUND
      REG_NR10  = 0xff10,
      REG_NR11  = 0xff11,
      REG_NR12  = 0xff12,
      REG_NR13  = 0xff13,
      REG_NR14  = 0xff14,
      REG_NR21  = 0xff16,
      REG_NR22  = 0xff17,
      REG_NR23  = 0xff18,
      REG_NR24  = 0xff19,
      REG_NR30  = 0xff1a,
      REG_NR31  = 0xff1b,
      REG_NR32  = 0xff1c,
      REG_NR33  = 0xff1d,
      REG_NR34  = 0xff1e,
      REG_NR41  = 0xff20,
      REG_NR42  = 0xff21,
      REG_NR43  = 0xff22,
      REG_NR44  = 0xff23,
      REG_NR50  = 0xff24,
      REG_NR51  = 0xff25,
      REG_NR52  = 0xff26,
      REG_WAVE  = 0xff30,
      // LCD
      REG_LCDC  = 0xff40,
      REG_STAT  = 0xff41,
      REG_SCY   = 0xff42,
      REG_SCX   = 0xff43,
      REG_LY    = 0xff44,
      REG_LYC   = 0xff45,
      REG_DMA   = 0xff46,
      // PALETTE
      REG_BGP   = 0xff47,
      REG_OBP0  = 0xff48,
      REG_OBP1  = 0xff49,
      REG_WY    = 0xff4a,
      REG_WX    = 0xff4b,

      REG_BOOT  = 0xff50,

      REG_HDMA1 = 0xff51,
      REG_HDMA2 = 0xff52,
      REG_HDMA3 = 0xff53,
      REG_HDMA4 = 0xff54,
      REG_HDMA5 = 0xff55,

      // INTERRUPTS
      REG_IF    = 0xff0f,
      REG_IE    = 0xffff,
    };
    enum key_t {
      KEY_RIGHT = 0x11, // bit0, bit4
      KEY_LEFT  = 0x12, // bit1, bit4
      KEY_UP    = 0x14, // bit2, bit4
      KEY_DOWN  = 0x18, // bit3, bit4
      KEY_A     = 0x21, // bit0, bit5
      KEY_B     = 0x22, // bit1, bit5
      KEY_START = 0x23, // bit2, bit5
      KEY_SELECT= 0x24, // bit3, bit5
    };

    IO(Machine&);
    void    write_io(const uint16_t, uint8_t);
    uint8_t read_io(const uint16_t);

    void    trigger(interrupt_t&);
    void    interrupt(interrupt_t&);
    uint8_t interrupt_mask();

    void trigger_key(key_t);
    bool is_vblank();

    Machine& machine() noexcept { return m_machine; }

    void reset();
    void simulate();

    inline uint8_t& reg(uint16_t addr) {
      return m_ioregs.at(addr & 0xff);
    }

    interrupt_t vblank;
    interrupt_t lcd_stat;
    interrupt_t timer;
    interrupt_t serial;
    interrupt_t joypad;
  private:
    Machine& m_machine;
    std::array<uint8_t, 128> m_ioregs = {};
    uint8_t  m_reg_ie = 0x0;
    uint8_t  m_ly = 0x0;
    uint64_t m_divider_time = 0;
  };
}
