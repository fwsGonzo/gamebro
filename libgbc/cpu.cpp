#include "cpu.hpp"

#include "machine.hpp"
#include <cassert>
#include <cstring>
#include "instructions.cpp"
#define INSTR(x) static instruction_t instr_##x {handler_##x, printer_##x}

namespace gbc
{
  INSTR(NOP);
  INSTR(LD_N_SP);
  INSTR(LD_R_N);
  INSTR(LD_D_N);
  INSTR(RST);
  INSTR(HALT);
  INSTR(STOP);
  INSTR(JP);
  INSTR(CALL);
  INSTR(MISSING);

  instruction_t& resolve_instruction(const uint8_t opcode)
  {
    if (opcode == 0) return instr_NOP;
    if (opcode == 0x08) return instr_LD_N_SP;
    if ((opcode & 0xcf) == 0x1) return instr_LD_R_N;
    if ((opcode & 0xc7) == 0x6) return instr_LD_D_N;
    if ((opcode & 0xc7) == 0xc7) return instr_RST;
    if ((opcode & 0xff) == 0xc3) return instr_JP; // direct
    if ((opcode & 0xc2) == 0xc2) return instr_JP; // conditional
    if ((opcode & 0xff) == 0xc4) return instr_CALL; // direct
    if ((opcode & 0xcd) == 0xcd) return instr_CALL; // conditional
    if ((opcode & 0xFF) == 0x10) return instr_STOP;

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
    if (registers().pc == 0x8) exit(1);
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
