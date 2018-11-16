#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include <util/delegate.hpp>
#include "common.hpp"
#include "mbc1.hpp"

namespace gbc
{
  class Memory
  {
  public:
    using range_t  = std::pair<uint16_t, uint16_t>;
    static constexpr range_t ProgramArea {0x0000, 0x7FFF};
    static constexpr range_t Display_Chr {0x8000, 0x97FF};
    static constexpr range_t Display_BG1 {0x9800, 0x9BFF};
    static constexpr range_t Display_BG2 {0x9C00, 0x9FFF};
    static constexpr range_t VideoRAM    {0x8000, 0x9FFF};

    static constexpr range_t BankRAM     {0xA000, 0xBFFF};
    static constexpr range_t WorkRAM     {0xC000, 0xDFFF};
    static constexpr range_t EchoRAM     {0xE000, 0xFDFF}; // echo of work RAM
    static constexpr range_t OAM_RAM     {0xFE00, 0xFEFF};

    static constexpr range_t IO_Ports    {0xFF00, 0xFF7F};
    static constexpr range_t ZRAM        {0xFF80, 0xFFFE};
    static constexpr uint16_t InterruptEn = 0xFFFF;

    Memory(Machine&, std::vector<uint8_t> rom);

    uint8_t read8(uint16_t address);
    void    write8(uint16_t address, uint8_t value);

    uint16_t read16(uint16_t address);
    void     write16(uint16_t address, uint16_t value);

    uint8_t* oam_ram_ptr() noexcept { return m_oam_ram.data(); }
    uint8_t* video_ram_ptr() noexcept { return m_video_ram.data(); }

    static constexpr uint16_t range_size(range_t range) { return range.second - range.first; }

    Machine& machine() noexcept { return m_machine; }
    void install_rom(std::vector<uint8_t> rom);
    void disable_bootrom();

    // debugging
    enum amode_t { READ, WRITE };
    using access_t = delegate<void(Memory&, uint16_t, uint8_t)>;
    void breakpoint(amode_t, access_t);

  private:
    inline bool is_within(uint16_t addr, const range_t& range) const
    {
      return addr >= range.first && addr <= range.second;
    }

    Machine& m_machine;
    MBC1                       m_mbc;
    std::array<uint8_t, 8192>  m_video_ram = {};
    std::array<uint8_t, 8192>  m_work_ram = {};
    std::array<uint8_t, 256>   m_oam_ram = {};
    std::array<uint8_t, 128>   m_zram = {}; // high-speed RAM
    std::vector<access_t> m_read_breakpoints;
    std::vector<access_t> m_write_breakpoints;
    bool m_bootrom_enabled = true;
  };

  inline void Memory::breakpoint(amode_t mode, access_t func) {
    if (mode == READ)
        m_read_breakpoints.push_back(func);
    else if (mode == WRITE)
        m_write_breakpoints.push_back(func);
  }
}
