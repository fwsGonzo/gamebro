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
    Machine(std::vector<uint8_t> rom);

    Memory memory;
    CPU    cpu;
    IO     io;
    DisplayData ddCharacter;
    DisplayData ddBackground1;
    DisplayData ddBackground2;

    uint64_t now() noexcept;

    // debugging aids
    void break_now();
    bool is_breaking() const noexcept;
    // make the machine stop when an undefined OP happens
    bool stop_when_undefined = false;
    bool break_on_interrupts = false;
    bool break_on_io = false;
    void undefined();
  };
}
