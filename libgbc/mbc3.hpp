#include "mbc1.hpp"

#include "machine.hpp"
#include "memory.hpp"

namespace gbc
{
  inline void MBC1::write_MBC3(uint16_t addr, uint8_t value)
  {
    switch (addr & 0xF000) {
      case 0x2000:
      case 0x3000:
          this->m_rom_bank_reg = value & 0x7F;
          this->set_rombank(this->m_rom_bank_reg);
          return;
      case 0x4000:
      case 0x5000:
          this->set_rambank(value & 0x7);
          this->m_rtc_enabled = (value & 0x80);
          return;
    }
  }
}
