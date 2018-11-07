// only include this file once!
#include "cpu.hpp"
#define INSTRUCTION(x) static unsigned instr_##x

namespace gbc
{
  INSTRUCTION(MISSING) (CPU& cpu, const instruction_t& instr)
  {
    cpu.registers().pc--;
    fprintf(stderr, "Missing instruction: %#x", cpu.readop8());
    assert(0 && "Unimplemented instruction reached");
  }

  INSTRUCTION(NOP) (CPU& cpu, const instruction_t&)
  {
    return 4; // NOP takes 4 T-states
  }

  INSTRUCTION(LD_BC) (CPU& cpu, const instruction_t&)
  {
    cpu.registers().bc = cpu.readop16(0);
    cpu.registers().pc += 1;
    return 4; // NOP takes 4 T-states
  }

  INSTRUCTION(LD_BC_A) (CPU& cpu, const instruction_t&)
  {
    cpu.registers().bc = cpu.registers().a;
    return 4; // NOP takes 4 T-states
  }

  INSTRUCTION(STOP) (CPU& cpu, const instruction_t&)
  {
    cpu.stop();
    return 4; // NOP takes 4 T-states
  }

  INSTRUCTION(RST) (CPU& cpu, const instruction_t& instr)
  {
    // jump to vector area
    cpu.registers().pc = instr.address;
    return 32; // RST takes xx T-states
  }
}
