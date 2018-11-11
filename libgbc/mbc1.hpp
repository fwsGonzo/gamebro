#pragma once
#include <array>
#include <cassert>
#include <cstdint>
#include <vector>

namespace gbc
{
  class Memory;

  class MBC1 {
  public:
    using range_t  = std::pair<uint16_t, uint16_t>;
    static constexpr range_t ROMbank0  {0x0000, 0x4000};
    static constexpr range_t ROMbankX  {0x4000, 0x8000};
    static constexpr range_t RAMbankX  {0xA000, 0xC000};

    MBC1(Memory&, std::vector<uint8_t> rom);

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
    Memory&  m_memory;
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
    printf("Setting new ROM bank offset to %#x\n", offset);
    assert((offset + rombank_size()) <= m_rom.size());
    this->m_rom_bank_offset = offset;
  }
  inline void MBC1::set_rambank(int offset)
  {
    offset = offset * rambank_size();
    printf("Setting new RAM bank offset to %#x\n", offset);
    assert((offset + rambank_size()) <= m_ram.size());
    this->m_ram_bank_offset = offset;
  }
}