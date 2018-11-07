#include "cpu.hpp"

#include "machine.hpp"
#include <cassert>
#include <cstring>
#include "instructions.cpp"

namespace gbc
{
  static std::array<instruction_t, 30> instructions {{
    {"NOP (0x0)",       0,       0, instr_NOP},      // 0x00
    {"LD (N), SP",       0,       0, instr_NOP},      // 0x00
  }};

  CPU::CPU(Memory& mem) noexcept
    : m_memory(mem)
  {
    this->reset();
  }

  void CPU::reset() noexcept
  {
    // gameboy Z80 initial register values
    registers().af = 0x01b0;
    registers().bc = 0x0013;
    registers().de = 0x00d8;
    registers().hl = 0x014d;
    registers().sp = 0xfffe;
    registers().pc = 0x0100;
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
  
  void CPU::stop()
  {
    this->m_running = false;
  }
}
