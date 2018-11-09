#include "memory.hpp"
#include "machine.hpp"

namespace gbc
{
  Memory::Memory(Machine& mach) : m_machine(mach) {}

  uint8_t Memory::read8(uint16_t address)
  {
    if (this->is_within(address, ProgramArea)) {
      if (address < 32768) {
        return m_rom.at(address - ProgramArea.first);
      }
      return m_rom.at(m_rom_bank + address - ProgramBank.first);
    }
    if (this->is_within(address, WorkRAM)) {
      return m_work_ram.at(address - WorkRAM.first);
    }
    if (this->is_within(address, BankRAM)) {
      return m_bank.at(m_ram_bank + address - BankRAM.first);
    }
    if (this->is_within(address, ZRAM)) {
      return m_zram.at(address - ZRAM.first);
    }
    char buffer[256];
    int len = snprintf(buffer, sizeof(buffer),
            "Invalid memory read at 0x%04x", address);
    throw std::runtime_error(std::string(buffer, len));
  }

  void Memory::write8(uint16_t address, uint8_t value)
  {
    if (this->is_within(address, ProgramArea)) {
      if (address < 32768) {
        m_rom.at(address - ProgramArea.first) = value;
      } else {
        m_rom.at(m_rom_bank + address - ProgramBank.first) = value;
      }
    }
    else if (this->is_within(address, WorkRAM)) {
      m_work_ram.at(address - WorkRAM.first) = value;
    }
    else if (this->is_within(address, BankRAM)) {
      m_bank.at(m_ram_bank + address - BankRAM.first) = value;
    }
    else if (this->is_within(address, ZRAM)) {
      m_zram.at(address - ZRAM.first) = value;
    }
    else {
      char buffer[256];
      int len = snprintf(buffer, sizeof(buffer),
              "Invalid memory write at 0x%04x, value 0x%x",
              address, value);
      throw std::runtime_error(std::string(buffer, len));
    }
  }

  uint16_t Memory::read16(uint16_t address) {
    return read8(address) | read8(address+1) << 8;
  }
  void Memory::write16(uint16_t address, uint16_t value) {
    write8(address+0, value & 0xff);
    write8(address+1, value >> 8);
  }
}
