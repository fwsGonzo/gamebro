#pragma once
#include <array>
#include <cassert>
#include <cstdint>
#include <vector>

namespace gbc
{
  class MBC1 {
  public:
    using range_t  = std::pair<uint16_t, uint16_t>;
    static constexpr range_t ROMbank0  {0x0000, 0x4000};
    static constexpr range_t ROMbankX  {0x4000, 0x8000};
    static constexpr range_t RAMbankX  {0xA000, 0xC000};

    MBC1(std::vector<uint8_t> rom)
        : m_rom(std::move(rom)) {}

    bool   rom_valid() const noexcept;
    bool   ram_enabled() const noexcept { return m_ram_enabled; }
    size_t rombank_size() const noexcept { return 0x4000; }
    size_t rambank_size() const noexcept { return 0x2000; }

    uint8_t read(uint16_t addr);
    void    write(uint16_t addr, uint8_t value);

    void set_rombank(int offset);
    void set_rambank(int offset);
    void set_mode(int mode);

  private:
    std::vector<uint8_t> m_rom;
    std::array<uint8_t, 32768> m_ram;
    uint32_t m_rom_bank_offset = 0x4000;
    uint16_t m_ram_bank_offset = 0x0;
    bool     m_ram_enabled = false;
    uint8_t  m_mode_select = 0x0;
  };

  inline bool MBC1::rom_valid() const noexcept
  {
    // TODO: implement me
    return true;
  }

  inline void MBC1::set_rombank(int offset)
  {
    // cant select bank 0
    offset = std::max(1, offset) * rombank_size();
    assert((offset + rombank_size()) <= m_rom.size());
    this->m_rom_bank_offset = offset;
  }
  inline void MBC1::set_rambank(int offset)
  {
    offset = offset * rambank_size();
    assert((offset + rambank_size()) <= m_ram.size());
    this->m_rom_bank_offset = offset;
  }

  inline uint8_t MBC1::read(uint16_t addr)
  {
    if (addr < ROMbank0.second)
    {
      return m_rom.at(addr);
    }
    else if (addr < ROMbankX.second)
    {
      addr -= ROMbankX.first;
      if (addr < rombank_size())
          return m_rom.at(m_rom_bank_offset + addr);
      else
          return 0xff;
    }
    else if (addr >= RAMbankX.first && addr < RAMbankX.second)
    {
      if (this->ram_enabled()) {
          addr -= RAMbankX.first;
          return m_ram.at(m_ram_bank_offset + addr);
      } else {
          return 0xff;
      }
    }
    printf("* Invalid MCB1 read: 0x%04x\n", addr);
    return 0xff;
  }
  inline void MBC1::write(uint16_t addr, uint8_t value)
  {
    if (addr < 0x2000) // RAM enable
    {
      this->m_ram_enabled = ((value & 0xF) == 0xA);
      return;
    }
    else if (addr < 0x4000) // ROM bank number
    {
      this->set_rombank(value & 0x1F);
      return;
    }
    else if (addr < 0x6000) // ROM/RAM bank number
    {
      this->set_rombank(value & 0xC0);
      return;
    }
    else if (addr < 0x8000) // ROM mode select
    {
      this->m_mode_select = value & 0x1;
      return;
    }
    printf("* Invalid MCB1 write: 0x%04x => 0x%02x\n", addr, value);
  }
}
