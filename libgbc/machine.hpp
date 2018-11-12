#pragma once

#include "cpu.hpp"
#include "gpu.hpp"
#include "memory.hpp"
#include "io.hpp"
#include "interrupt.hpp"

namespace gbc
{
  class Machine
  {
  public:
    Machine(std::vector<uint8_t> rom);

    CPU    cpu;
    Memory memory;
    IO     io;
    GPU    gpu;

    uint64_t now() noexcept;

    // set delegates to be notified on interrupts
    enum interrupt {
      VBLANK,
      TIMER
    };
    void set_handler(interrupt, interrupt_handler);

    /// debugging aids ///
    bool verbose_instructions = false;
    // make the machine stop when an undefined OP happens
    bool stop_when_undefined = false;
    bool break_on_interrupts = false;
    bool break_on_io = false;
    void break_now();
    bool is_breaking() const noexcept;
    void undefined();
  };
}
