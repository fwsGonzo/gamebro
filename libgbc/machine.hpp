#pragma once

#include "cpu.hpp"
#include "display_data.hpp"
#include "memory.hpp"
#include "io.hpp"

namespace gbc
{
  class Machine
  {
  public:
    Machine(const std::vector<uint8_t>& rom);

    Memory memory;
    CPU    cpu;
    IO     io;
    DisplayData ddCharacter;
    DisplayData ddBackground1;
    DisplayData ddBackground2;

    // make the machine stop when an undefined OP happens
    bool stop_when_undefined = false;
    void undefined();
  };
}
