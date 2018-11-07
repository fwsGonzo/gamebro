#include "cpu.hpp"

#include "machine.hpp"
#include <cassert>
#include <cstring>
#include "instructions.cpp"

namespace gbc
{
  static std::array<instruction_t, 3> instructions {{
    {"NOP",            0, instr_NOP},      // 0x00
    {"LD BC, 0x%04x",  2, instr_LD_BC},    // 0x01
    {"LD (BC), A",     0, instr_LD_BC_A},  // 0x02
  }};

  CPU::CPU(Memory& mem) noexcept
    : m_memory(mem)
  {
    this->reset();
  }

  void CPU::reset() noexcept
  {
    std::memset(&registers(), 0, sizeof(regs_t));
    this->m_cycles_total = 0;
  }

  uint8_t CPU::readop8(const int dx)
  {
    return memory().read8(registers().pc + dx);
  }
  uint16_t CPU::readop16(const int dx)
  {
    return memory().read16(registers().pc + dx);
  }

  void CPU::simulate()
  {
    // 1. read instruction from memory
    const uint8_t opcode = this->readop8(0);
    auto& instr = instructions.at(opcode);
    printf("Executing opcode %#x: %s\n",
           opcode, instr.mnemonic.c_str());
    // 2. increment program counter
    registers().pc += 1;
    if (registers().pc == 0x8) exit(1);
    // 3. execute opcode
    unsigned time = this->execute(instr);
    // 4. pass the time (in T-states)
    this->incr_cycles(time);
  }

  unsigned CPU::execute(const instruction_t& instr)
  {
    return instr.handler(*this, instr);
  }

  void CPU::incr_cycles(int count)
  {
    assert(count >= 0);
    this->m_cycles_total += count;
  }
}
