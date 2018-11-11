#include "memory.hpp"
#include "machine.hpp"

namespace gbc
{
  Memory::Memory(Machine& mach, std::vector<uint8_t> rom)
    : m_machine(mach), m_mbc{*this, std::move(rom)}
  {
    assert(m_mbc.rom_valid());
  }

  uint8_t Memory::read8(uint16_t address)
  {
    for (auto& func : m_read_breakpoints) {
      func(*this, address, 0x0);
    }
    if (this->is_within(address, ProgramArea)) {
      return m_mbc.read(address);
    }
    else if (this->is_within(address, VideoRAM)) {
      return m_video_ram.at(address - VideoRAM.first);
    }
    else if (this->is_within(address, WorkRAM)) {
      return m_work_ram.at(address - WorkRAM.first);
    }
    else if (this->is_within(address, EchoRAM)) {
      return m_work_ram.at(address - EchoRAM.first);
    }
    else if (this->is_within(address, BankRAM)) {
      return m_mbc.read(address);
    }
    else if (this->is_within(address, IO_Ports)) {
      return machine().io.read_io(address);
    }
    else if (this->is_within(address, ZRAM)) {
      return m_zram.at(address - ZRAM.first);
    }
    else if (address == InterruptEn) {
      return machine().io.read_io(address);
    }
    printf(">>> Invalid memory read at 0x%04x\n", address);
    return 0xff;
  }

  void Memory::write8(uint16_t address, uint8_t value)
  {
    for (auto& func : m_write_breakpoints) {
      func(*this, address, value);
    }
    if (this->is_within(address, ProgramArea)) {
      m_mbc.write(address, value);
      return;
    }
    else if (this->is_within(address, VideoRAM)) {
      m_video_ram.at(address - VideoRAM.first) = value;
      return;
    }
    else if (this->is_within(address, WorkRAM)) {
      m_work_ram.at(address - WorkRAM.first) = value;
      return;
    }
    else if (this->is_within(address, EchoRAM)) {
      m_work_ram.at(address - EchoRAM.first) = value;
      return;
    }
    else if (this->is_within(address, BankRAM)) {
      m_mbc.write(address, value);
      return;
    }
    else if (this->is_within(address, IO_Ports)) {
      machine().io.write_io(address, value);
      return;
    }
    else if (this->is_within(address, ZRAM)) {
      m_zram.at(address - ZRAM.first) = value;
      return;
    }
    else if (address == InterruptEn) {
      machine().io.write_io(address, value);
      return;
    }
    printf(">>> Invalid memory write at 0x%04x, value 0x%x\n",
           address, value);
  }

  uint16_t Memory::read16(uint16_t address) {
    return read8(address) | read8(address+1) << 8;
  }
  void Memory::write16(uint16_t address, uint16_t value) {
    write8(address+0, value & 0xff);
    write8(address+1, value >> 8);
  }
}
