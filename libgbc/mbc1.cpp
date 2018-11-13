#include "mbc1.hpp"

#include "machine.hpp"
#include "memory.hpp"

namespace gbc
{
  MBC1::MBC1(Memory& m, std::vector<uint8_t> rom)
      : m_memory(m), m_rom(std::move(rom)) {}

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
          return m_ram.at(m_ram_bank_offset | addr);
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
      return;
    }
    else if (addr < 0x4000) // ROM bank number
    {
      this->set_rombank(value & 0x1F);
      //m_memory.machine().break_now();
      return;
    }
    else if (addr < 0x6000) // ROM/RAM bank number
    {
      this->set_rombank(value & 0x60);
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

  void MBC1::set_rombank(int offset)
  {
    // cant select bank 0
    offset = std::max(1, offset) * rombank_size();
    printf("Setting new ROM bank offset to %#x\n", offset);
    assert((offset + rombank_size()) <= m_rom.size());
    this->m_rom_bank_offset = offset;
  }
  void MBC1::set_rambank(int offset)
  {
    offset = offset * rambank_size();
    printf("Setting new RAM bank offset to %#x\n", offset);
    assert((offset + rambank_size()) <= m_ram.size());
    this->m_ram_bank_offset = offset;
  }
  void MBC1::set_mode(int mode)
  {
    this->m_mode_select = mode & 0x1;
    printf("Mode select: 0x%02x\n", this->m_mode_select);
    this->m_memory.machine().break_now();
  }
}
