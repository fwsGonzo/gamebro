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
    static constexpr range_t WRAM_0    {0xC000, 0xD000};
    static constexpr range_t WRAM_bX   {0xD000, 0xE000};
    static constexpr range_t EchoRAM   {0xE000, 0xFE00};

    MBC1(Memory&, const std::vector<uint8_t>& rom);

    bool   ram_enabled() const noexcept   { return m_ram_enabled; }
    size_t rombank_size() const noexcept  { return 0x4000; }
    size_t rambank_size() const noexcept  { return 0x2000; }
    size_t wrambank_size() const noexcept { return 0x1000; }

    uint8_t read(uint16_t addr);
    void    write(uint16_t addr, uint8_t value);

    void set_rombank(int offset);
    void set_rambank(int offset);
    void set_wrambank(int offset);
    void set_mode(int mode);

  private:
    void write_MBC1M(uint16_t, uint8_t);
    void write_MBC3(uint16_t, uint8_t);
    void write_MBC5(uint16_t, uint8_t);
    bool verbose_banking() const noexcept;

    Memory&  m_memory;
    const std::vector<uint8_t>& m_rom;
    std::array<uint8_t, 131072> m_ram;
    std::array<uint8_t, 32768>  m_wram;
    uint32_t m_rom_bank_offset = 0x4000;
    uint16_t m_ram_banks       = 0;
    uint32_t m_ram_bank_size   = 0x0;
    uint16_t m_ram_bank_offset = 0x0;
    uint16_t m_wram_offset  = 0x1000;
    uint16_t m_wram_size    = 0x2000;
    bool     m_ram_enabled  = false;
    bool     m_rtc_enabled  = false;
    uint16_t m_rom_bank_reg = 0x1;
    uint8_t  m_mode_select  = 0;
    uint8_t  m_version = 1;
  };
}
