#pragma once
#include <array>
#include <cstdint>
#include <util/delegate.hpp>
#include "registers.hpp"
#include "instruction.hpp"

namespace gbc
{
  class Memory;

  class CPU
  {
  public:
    CPU(Memory&) noexcept;
    void  reset() noexcept;
    void  simulate();

    uint8_t  readop8(int dx = 0);
    uint16_t readop16(int dx = 0);
    unsigned execute(const uint8_t);
    void     incr_cycles(int count);
    void     stop();
    void     wait();

    regs_t& registers() noexcept { return m_registers; }

    Memory& memory() { return m_memory; }

    bool is_running() const noexcept { return m_running; }
    bool is_waiting() const noexcept { return m_waiting; }

  private:
    Memory&  m_memory;
    regs_t   m_registers;
    uint64_t m_cycles_total = 0;
    bool     m_running = true;
    bool     m_waiting = false;
  };
}
