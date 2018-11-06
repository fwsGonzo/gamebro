#include "memory.hpp"
#include "machine.hpp"

namespace gbc
{
  Memory::Memory(Machine& mach) : m_machine(mach) {}

  uint8_t Memory::read(uint16_t address)
  {
    m_machine.incr_cycles(1);
    if (this->is_within(address, ProgramArea)) {
      return m_program_area.at(address - ProgramArea.first);
    }
    if (this->is_within(address, WorkRAM)) {
      return m_work_ram.at(address - WorkRAM.first);
    }
    throw std::runtime_error("Invalid memory read at address " +
                              std::to_string(address));
  }

  void Memory::write(uint16_t address, uint8_t value)
  {
    m_machine.incr_cycles(1);
    if (this->is_within(address, ProgramArea)) {
      m_program_area.at(address - ProgramArea.first) = value;
    }
    else if (this->is_within(address, WorkRAM)) {
      m_work_ram.at(address - WorkRAM.first) = value;
    }
    else {
      throw std::runtime_error("Invalid memory read at address " +
                               std::to_string(address));
    }
  }
}
