#pragma once

#include "cpu.hpp"
#include "display_data.hpp"
#include "memory.hpp"

namespace gbc
{
  class Machine
  {
  public:
    Machine(const std::vector<uint8_t>& rom);

    Memory memory;
    CPU    cpu;
    DisplayData ddCharacter;
    DisplayData ddBackground1;
    DisplayData ddBackground2;
  };
}
