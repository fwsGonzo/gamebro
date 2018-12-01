#include "mbc.hpp"

#include "machine.hpp"
#include "memory.hpp"

#include "mbc1m.hpp"
#include "mbc3.hpp"
#include "mbc5.hpp"

namespace gbc
{
  MBC::MBC(Memory& m, const std::vector<uint8_t>& rom)
      : m_memory(m), m_rom(rom)
  {
    m_ram.at(0x100) = 0x1;
    m_ram.at(0x101) = 0x3;
    m_ram.at(0x102) = 0x5;
    m_ram.at(0x103) = 0x7;
    m_ram.at(0x104) = 0x9;
    // test ROMs are just instruction arrays
    if (m_rom.size() < 0x150) return;
    // parse ROM header
    switch (m.read8(0x147)) {
      case 0x0:
      case 0x1: // MBC 1
      case 0x2:
      case 0x3:
          this->m_version = 1;
          break;
      case 0x5:
      case 0x6:
          this->m_version = 2;
          assert(0 && "MBC2 is a weirdo!");
          break;
      case 0x0F:
      case 0x10: // MBC 3
      case 0x12:
      case 0x13:
          this->m_version = 3;
          break;
      case 0x19:
      case 0x1A: // MBC 5
      case 0x1B:
      case 0x1C:
      case 0x1D:
      case 0x1E:
          this->m_version = 5;
          break;
      default:
          assert(0 && "Unknown cartridge type");
    }
    printf("MBC version %u\n", this->m_version);
    switch (m.read8(0x149)) {
      case 0x0:
          m_ram_banks = 0;
          m_ram_bank_size = 0;
          break;
      case 0x1: // 2kb
          m_ram_banks = 1;
          m_ram_bank_size = 2048;
          break;
      case 0x2: // 8kb
          m_ram_banks = 1;
          m_ram_bank_size = 8192;
          break;
      case 0x3: // 32kb
          m_ram_banks = 4;
          m_ram_bank_size = 32768;
          break;
      case 0x4: // 128kb
          m_ram_banks = 16;
          m_ram_bank_size = 0x20000;
          break;
      case 0x5: // 64kb
          m_ram_banks = 8;
          m_ram_bank_size = 0x10000;
          break;
    }
    printf("RAM bank size: 0x%05x\n", m_ram_bank_size);
    m_wram_size = 0x2000;
    printf("Work RAM bank size: 0x%04x\n", m_wram_size);
  }

  uint8_t MBC::read(uint16_t addr)
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
        if (this->m_rtc_enabled == false)
        {
          addr -= RAMbankX.first;
          addr |= m_ram_bank_offset;
          if (addr < m_ram_bank_size)
              return m_ram.at(addr);
          return 0xff; // small 2kb RAM banks
        }
        else {
          return 0xff; // TODO: Read from RTC register
        }
      } else {
          return 0xff;
      }
    }
    else if (addr >= WRAM_0.first && addr < WRAM_0.second)
    {
      return m_wram.at(addr - WRAM_0.first);
    }
    else if (addr >= WRAM_bX.first && addr < WRAM_bX.second)
    {
      return m_wram.at(m_wram_offset + addr - WRAM_bX.first);
    }
    else if (addr >= EchoRAM.first && addr < EchoRAM.second)
    {
      return this->read(addr - 0x2000);
    }
    printf("* Invalid MBC read: 0x%04x\n", addr);
    return 0xff;
  }

  void MBC::write(uint16_t addr, uint8_t value)
  {
    if (addr < 0x2000) // RAM enable
    {
      bool old_ram_enabled = this->m_ram_enabled;
      if (m_version == 2)
          this->m_ram_enabled = value != 0;
      else
          this->m_ram_enabled = ((value & 0xF) == 0xA);
      if (UNLIKELY(verbose_banking() && old_ram_enabled != m_ram_enabled)) {
          printf("* External RAM enabled: %d\n", this->m_ram_enabled);
      }
      return;
    }
    else if (addr < 0x8000)
    {
      switch (this->m_version) {
        case 5:
            this->write_MBC5(addr, value); break;
        case 3:
            this->write_MBC3(addr, value); break;
        case 1:
            this->write_MBC1M(addr, value); break;
        case 0:
            break; // no MBC
        default:
            assert(0 && "Unimplemented MBC version");
      }
      return;
    }
    else if (addr >= RAMbankX.first && addr < RAMbankX.second)
    {
      if (this->ram_enabled()) {
          if (this->m_rtc_enabled == false)
          {
            addr -= RAMbankX.first;
            this->m_ram.at(m_ram_bank_offset | addr) = value;
          }
          else {
            // TODO: Write to RTC register
          }
      }
      return;
    }
    else if (addr >= WRAM_0.first && addr < WRAM_0.second)
    {
      this->m_wram.at(addr - WRAM_0.first) = value;
      return;
    }
    else if (addr >= WRAM_bX.first && addr < WRAM_bX.second)
    {
      this->m_wram.at(m_wram_offset + addr - WRAM_bX.first) = value;
      return;
    }
    else if (addr >= EchoRAM.first && addr < EchoRAM.second)
    {
      this->write(addr - 0x2000, value);
      return;
    }
    printf("* Invalid MBC write: 0x%04x => 0x%02x\n", addr, value);
    assert(0);
  }

  void MBC::set_rombank(int reg)
  {
    if (reg == 0) reg = 1;
    if (this->m_version < 3) { // bug!
      if (reg == 0x20 || reg == 0x40 || reg == 0x60) reg++;
    }
    // cant select bank 0
    const int offset = reg * rombank_size();
    if (verbose_banking()) {
      printf("Selecting ROM bank 0x%02x offset %#x max %#zx\n", reg, offset, m_rom.size());
    }
    if (UNLIKELY((offset + rombank_size()) > m_rom.size()))
    {
      printf("Invalid ROM bank 0x%02x offset %#x max %#zx\n", reg, offset, m_rom.size());
      this->m_memory.machine().break_now();
      return;
    }
    this->m_rom_bank_offset = offset;
  }
  void MBC::set_rambank(int reg)
  {
    // NOTE: we have to remove bits here
    reg &= (this->m_ram_banks-1);
    const int offset = reg * rambank_size();
    if (UNLIKELY(verbose_banking())) {
      printf("Selecting RAM bank 0x%02x offset %#x max %#x\n", reg, offset, m_ram_bank_size);
    }
    if (UNLIKELY((offset + rambank_size()) > m_ram_bank_size))
    {
      printf("Invalid RAM bank 0x%02x offset %#x\n", reg, offset);
      this->m_memory.machine().break_now();
      return;
    }
    this->m_ram_bank_offset = offset;
  }
  void MBC::set_wrambank(int reg)
  {
    const int offset = reg * wrambank_size();
    if (UNLIKELY(verbose_banking())) {
      printf("Selecting WRAM bank 0x%02x offset %#x max %#x\n", reg, offset, m_wram_size);
    }
    if (UNLIKELY((offset + wrambank_size()) > m_wram_size))
    {
      printf("Invalid Work RAM bank 0x%02x offset %#x\n", reg, offset);
      this->m_memory.machine().break_now();
      return;
    }
    if (UNLIKELY(verbose_banking())) {
      printf("Work RAM bank 0x%02x offset %#x\n", reg, offset);
    }
    this->m_wram_offset = offset;
  }
  void MBC::set_mode(int mode)
  {
    if (UNLIKELY(verbose_banking())) {
      printf("Mode select: 0x%02x\n", this->m_mode_select);
    }
    this->m_mode_select = mode & 0x1;
    // for MBC we have to reset the upper bits when going into RAM mode
    if (this->m_version == 1 && this->m_mode_select == 1)
    {
      // reset ROM bank upper bits when going into RAM mode
      if (this->m_rom_bank_reg & 0x60) {
          this->m_rom_bank_reg &= 0x1F;
          this->set_rombank(this->m_rom_bank_reg);
      }
    }
  }

  bool MBC::verbose_banking() const noexcept {
    return m_memory.machine().verbose_banking;
  }
}
