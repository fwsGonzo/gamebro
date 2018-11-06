#include "cpu.hpp"

#include "machine.hpp"
#include "operations.hpp"
#include <cassert>

namespace gbc
{
  CPU::CPU(Machine& machine) noexcept
    : m_machine(machine)
  {
    this->reset();
    // install opcodes
    ops[0] = instr_NOP;
  }

  void CPU::reset() noexcept
  {
    this->m_regs = {0};
    this->sp_set(0x0);
    this->pc_set(0x0);
    this->m_cycles_total = 0;
  }

  uint8_t CPU::readop(int dx) const
  {
    return m_machine.memory.read8(m_pc + dx);
  }

  void CPU::simulate()
  {
    // 1. read instruction from memory
    uint8_t opcode = this->readop(0);
    // 2. increment program counter
    this->pc_set(this->pc_read() + 1);
    // 3. execute opcode
    this->execute(opcode);
    // 4. pass the time
    this->incr_cycles(reg_read(REG_T));
  }

  void CPU::execute(uint8_t opcode)
  {
    if (ops.at(opcode) != nullptr) {
        ops[opcode](*this, opcode);
        return;
    }

    printf("WARNING: opcode %#x treated as NOP\n", opcode);
    instr_NOP(*this, opcode);
  }

  void CPU::incr_cycles(int count)
  {
    assert(count >= 0);
    this->m_cycles_total += count;
  }

}
