#include "cpu.hpp"

#include "machine.hpp"
#include <cassert>

namespace gbc
{
  CPU::CPU(Machine& machine)
    : m_machine(machine)
  {

  }

  void CPU::simulate()
  {
    // 1. read from memory at PC
    uint8_t opcode = m_machine.memory.read(this->m_pc);
    // 2. execute opcode
    const uint8_t len = this->execute(opcode);
    assert(len > 0 && len < 4);
    // 3. increment program counter
    this->m_pc += len;
    // 4. record how much time it took
    this->incr_cycles(1);
    asm("pause");
  }

  uint8_t CPU::execute(uint8_t opcode)
  {
    if (ops.at(opcode) != nullptr)
        return ops[opcode](*this, opcode);
    throw std::runtime_error("Missing opcode: " + std::to_string(opcode));
  }

  void CPU::incr_cycles(int count)
  {
    assert(count >= 0);
    this->m_cycles_last = count;
    this->m_cycles_total += count;
  }

}
