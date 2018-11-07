// only include this file once!
#include "cpu.hpp"
#define INSTRUCTION(x) static unsigned instr_##x

namespace gbc
{
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
}
