#pragma once
#include <array>
#include <map>
#include <cassert>
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
    using breakpoint_t = delegate<void(CPU&, uint8_t)>;
    CPU(Memory&) noexcept;
    void  reset() noexcept;
    void  simulate();
    uint64_t gettime() const noexcept { return m_cycles_total; }

    instruction_t& resolve_instruction(uint8_t opcode);
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

    // debugging
    void breakpoint(uint16_t address, breakpoint_t func);
    void default_pausepoint(uint16_t address, bool single_step);
    void single_step(bool en) { m_singlestep = en; }
    static void print_and_pause(CPU&, const uint8_t opcode);

    std::string to_string() const;

  private:
    regs_t   m_registers;
    Memory&  m_memory;
    uint64_t m_cycles_total = 0;
    uint8_t  m_last_flags = 0xFF;
    bool     m_running = true;
    bool     m_waiting = false;
    bool     m_singlestep = false;
    std::map<uint16_t, breakpoint_t> m_breakpoints;
  };

  inline void CPU::breakpoint(uint16_t addr, breakpoint_t func)
  {
    assert(func != nullptr);
    this->m_breakpoints[addr] = func;
  }

  inline void CPU::default_pausepoint(const uint16_t address, bool ss)
  {
    this->breakpoint(address,
      [ss] (gbc::CPU& cpu, const uint8_t opcode) {
        print_and_pause(cpu, opcode);
        cpu.single_step(ss);
      });
  }
}
