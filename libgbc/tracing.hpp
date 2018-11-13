#pragma once
#include <cstdint>
#include <util/delegate.hpp>

namespace gbc
{
  class CPU;

  struct breakpoint_t {
    delegate<void(CPU&, uint8_t)> callback;
    int  break_on_steps = 0;
    bool verbose_instr = false;
  };

}
