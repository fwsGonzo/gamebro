#pragma once

#include "cpu.hpp"
#include "gpu.hpp"
#include "memory.hpp"
#include "io.hpp"
#include "interrupt.hpp"

namespace gbc
{
  enum keys_t {
    DPAD_RIGHT = 0x1,
    DPAD_LEFT  = 0x2,
    DPAD_UP    = 0x4,
    DPAD_DOWN  = 0x8,
    BUTTON_A   = 0x10,
    BUTTON_B   = 0x20,
    BUTTON_SELECT = 0x40,
    BUTTON_START  = 0x80
  };

  class Machine
  {
  public:
    Machine(std::vector<uint8_t> rom, bool init = true);

    CPU    cpu;
    Memory memory;
    IO     io;
    GPU    gpu;

    void     reset();
    uint64_t now() noexcept;

    // set delegates to be notified on interrupts
    enum interrupt {
      VBLANK,
      TIMER,
      DEBUG
    };
    void set_handler(interrupt, interrupt_handler);

    // use keys_t to form an 8-bit mask
    void set_inputs(uint8_t mask);

    /// debugging aids ///
    bool verbose_instructions = false;
    bool verbose_interrupts   = false;
    // make the machine stop when an undefined OP happens
    bool stop_when_undefined = false;
    bool break_on_interrupts = false;
    bool break_on_io = false;
    void break_now();
    bool is_breaking() const noexcept;
    void undefined();
  };
}
