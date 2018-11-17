#include "mbc1.hpp"

#include "machine.hpp"
#include "memory.hpp"

namespace gbc
{
  MBC1::MBC1(Memory& m, std::vector<uint8_t> rom)
      : m_memory(m), m_rom(std::move(rom))
  {
    m_ram.at(0x100) = 0x1;
    m_ram.at(0x101) = 0x3;
    m_ram.at(0x102) = 0x5;
    m_ram.at(0x103) = 0x7;
    m_ram.at(0x104) = 0x9;
    switch (m.read8(0x149)) {
      case 0x0:
          m_ram_bank_size = 0; break;
      case 0x1: // 2kb
          m_ram_bank_size = 2048; break;
      case 0x2: // 8kb
          m_ram_bank_size = 8192; break;
      case 0x3: // 32kb
          m_ram_bank_size = 32768; break;
      case 0x4: // 128kb
          m_ram_bank_size = 0x20000; break;
      case 0x5: // 64kb
          m_ram_bank_size = 0x10000; break;
    }
    printf("RAM bank size: 0x%05x\n", m_ram_bank_size);
  }
  void MBC1::install_rom(std::vector<uint8_t> rom) {
    this->m_rom = std::move(rom);
  }

  uint8_t MBC1::read(uint16_t addr)
  {
    if (addr < ROMbank0.second)
    {
      return m_rom.at(addr);
    }
    else if (addr < ROMbankX.second)
    {
      addr -= ROMbankX.first;
      if (addr < rombank_size()) {
          //printf("Reading ROM bank at %#x\n", m_rom_bank_offset | addr);
          return m_rom.at(m_rom_bank_offset | addr);
      } else {
          return 0xff;
      }
    }
    else if (addr >= RAMbankX.first && addr < RAMbankX.second)
    {
      if (this->ram_enabled()) {
          addr -= RAMbankX.first;
          if (addr < m_ram_bank_size)
              return m_ram.at(m_ram_bank_offset | addr);
          return 0xff; // small 2kb RAM banks
      } else {
          return 0xff;
      }
    }
    printf("* Invalid MCB1 read: 0x%04x\n", addr);
    return 0xff;
  }

  void MBC1::write(uint16_t addr, uint8_t value)
  {
    if (addr < 0x2000) // RAM enable
    {
      this->m_ram_enabled = ((value & 0xF) == 0xA);
      //printf("* RAM enabled: %d\n", this->m_ram_enabled);
      return;
    }
    else if (addr < 0x4000) // ROM bank number
    {
      this->m_rom_bank_reg &= 0x60;
      this->m_rom_bank_reg |= value & 0x1F;
      this->set_rombank(this->m_rom_bank_reg);
      return;
    }
    else if (addr < 0x6000) // ROM/RAM bank number
    {
      if (this->m_ram_bank_size > 0) {
        this->set_rambank(value & 0x3);
      }
      else {
        this->m_rom_bank_reg &= 0x1F;
        this->m_rom_bank_reg |= value & 0x60;
        this->set_rombank(this->m_rom_bank_reg);
      }
      return;
    }
    else if (addr < 0x8000) // ROM/RAM mode select
    {
      this->set_mode(value);
      return;
    }
    else if (addr >= RAMbankX.first && addr < RAMbankX.second)
    {
      if (this->ram_enabled()) {
          addr -= RAMbankX.first;
          this->m_ram.at(m_ram_bank_offset | addr) = value;
      }
      return;
    }
    printf("* Invalid MCB1 write: 0x%04x => 0x%02x\n", addr, value);
    assert(0);
  }

  bool MBC1::rom_valid() const noexcept
  {
    // TODO: implement me
    return true;
  }

  void MBC1::set_rombank(int reg)
  {
    if (reg == 0x0 || reg == 0x20 || reg == 0x40 || reg == 0x60) reg++;
    // cant select bank 0
    const int offset = reg * rombank_size();
    if (UNLIKELY((offset + rombank_size()) > m_rom.size()))
    {
      printf("Invalid ROM bank 0x%02x offset %#x max %#zx\n", reg, offset, m_rom.size());
      this->m_memory.machine().break_now();
      return;
    }
    this->m_rom_bank_offset = offset;
  }
  void MBC1::set_rambank(int reg)
  {
    const int offset = reg * rambank_size();
    if (UNLIKELY((offset + rambank_size()) > m_ram_bank_size))
    {
      printf("Invalid RAM bank 0x%02x offset %#x\n", reg, offset);
      this->m_memory.machine().break_now();
      return;
    }
    this->m_ram_bank_offset = offset;
  }
  void MBC1::set_mode(int mode)
  {
    this->m_mode_select = mode & 0x1;
    printf("Mode select: 0x%02x\n", this->m_mode_select);
    this->m_memory.machine().break_now();
  }
}
