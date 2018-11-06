#pragma once
#include <array>
#include <cstdint>
#include <util/delegate.hpp>

namespace gbc
{
  class Machine;

  class CPU
  {
  public:
    using opcode_t = delegate<uint8_t(CPU&, uint8_t instr)>;
    enum regs_t {
      REG_A = 0,
      REG_B,
      REG_C,
      REG_D,
      REG_E,
      REG_F,
      REG_H,
      REG_I,
      REG_SP,
      REGS
    };

    CPU(Machine&);
    void    simulate();
    uint8_t execute(uint8_t opcode);
    void    incr_cycles(int count);

  private:
    Machine& m_machine;
    std::array<uint8_t, REGS> m_regs = {0};
    uint16_t m_pc = 0x0;
    uint16_t m_cycles_last = 0;
    uint64_t m_cycles_total = 0;

    std::array<opcode_t, 256> ops;
  };
}
