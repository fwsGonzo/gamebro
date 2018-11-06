#pragma once
#include "cpu.hpp"

namespace gbc
{
  inline void instr_NOP(CPU& cpu, const uint8_t)
  {
    cpu.reg_set(CPU::REG_M, 1);
    cpu.reg_set(CPU::REG_T, 4);
  }
}
