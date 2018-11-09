#include "cpu.hpp"

#include "machine.hpp"
#include <cassert>
#include <cstring>
#include "instructions.cpp"

namespace gbc
{
  instruction_t& resolve_instruction(const uint8_t opcode)
  {
    if (opcode == 0) return instr_NOP;
    if (opcode == 0x08) return instr_LD_N_SP;


    if ((opcode & 0xc0) == 0x40) {
      if (opcode == 0x76) return instr_HALT;
      return instr_LD_D_D;
    }
    if ((opcode & 0xcf) == 0x1)  return instr_LD_R_N;
    if ((opcode & 0xe7) == 0x2)  return instr_LD_R_A_R;
    if ((opcode & 0xc7) == 0x3)  return instr_INC_DEC_R;
    if ((opcode & 0xc6) == 0x4)  return instr_INC_DEC_D;
    if (opcode == 0x10) return instr_STOP;
    if (opcode == 0x18) return instr_JR_N;
    if ((opcode & 0xc7) == 0x6)  return instr_LD_D_N;
    if ((opcode & 0xe7) == 0x22) return instr_LDID_HL_A;
    if ((opcode & 0xc6) == 0xc6) return instr_ALU_A_N_D;
    if ((opcode & 0xc0) == 0x80) return instr_ALU_A_N_D;
    if ((opcode & 0xcb) == 0xc1) return instr_PUSH_POP;
    if ((opcode & 0xe7) == 0xc0) return instr_RET; // cond ret
    if (opcode == 0xc9) return instr_RET;          // plain ret
    if ((opcode & 0xc7) == 0xc7) return instr_RST;
    if ((opcode & 0xff) == 0xc3) return instr_JP; // direct
    if ((opcode & 0xe7) == 0xc2) return instr_JP; // conditional
    if ((opcode & 0xff) == 0xc4) return instr_CALL; // direct
    if ((opcode & 0xcd) == 0xcd) return instr_CALL; // conditional
    if ((opcode & 0xef) == 0xea) return instr_LD_N_A_N;

    return instr_MISSING;
  }

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
    // 2. execute instruction
    unsigned time = this->execute(opcode);
    // 3. pass the time (in T-states)
    this->incr_cycles(time);
  }

  unsigned CPU::execute(const uint8_t opcode)
  {
    char prn[128];
    auto& instr = resolve_instruction(opcode);
    instr.printer(prn, sizeof(prn), *this, opcode);
    printf("[pc 0x%04x] opcode 0x%02x: %s\n",
            registers().pc,  opcode, prn);
    // increment program counter
    registers().pc += 1;
    static int counter = 0;
    if (counter++ == 2000) assert(0);
    // run instruction handler
    return instr.handler(*this, opcode);
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

  void CPU::wait()
  {
    this->m_waiting = true;
  }
}
