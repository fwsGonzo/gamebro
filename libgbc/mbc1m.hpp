#include "mbc1.hpp"

#include "machine.hpp"
#include "memory.hpp"

namespace gbc
{
  inline void MBC1::write_MBC1M(uint16_t addr, uint8_t value)
  {
    switch (addr & 0xF000)
    {
      case 0x2000:
      case 0x3000:
          // ROM bank number
          this->m_rom_bank_reg &= 0x60;
          this->m_rom_bank_reg |= value & 0x1F;
          this->set_rombank(this->m_rom_bank_reg);
          return;
      case 0x4000:
      case 0x5000:
          // ROM / RAM bank select
          if (this->m_mode_select == 1) {
            this->set_rambank(value & 0x3);
          }
          else {
            this->m_rom_bank_reg &= 0x1F;
            this->m_rom_bank_reg |= value & 0x60;
            this->set_rombank(this->m_rom_bank_reg);
          }
          return;
      case 0x6000:
      case 0x7000:
          // RAM / ROM mode select
          this->set_mode(value & 0x1);
    }
  }
} // gbc
