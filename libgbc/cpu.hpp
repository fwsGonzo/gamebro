#pragma once
#include <array>
#include <map>
#include <cassert>
#include <cstdint>
#include "instruction.hpp"
#include "registers.hpp"
#include "tracing.hpp"

namespace gbc
{
  class Machine;
  class Memory;

  class CPU
  {
  public:
    CPU(Machine&, bool init) noexcept;
    void  reset() noexcept;
    void  simulate();
    uint64_t gettime() const noexcept { return m_cycles_total; }

    uint8_t  current_opcode() const noexcept { return m_cur_opcode; }
    uint8_t  readop8(int dx = 0);
    uint16_t readop16(int dx = 0);
    unsigned execute(const uint8_t);
    unsigned push_and_jump(uint16_t addr);
    void     incr_cycles(int count);
    void     stop();
    void     wait(); // wait for interrupts
    instruction_t& decode(uint8_t opcode);

    regs_t& registers() noexcept { return m_registers; }
    // helpers for reading and writing (HL)
    uint8_t read_hl();
    void    write_hl(uint8_t);

    Memory&  memory() noexcept { return m_memory; }
    Machine& machine() noexcept { return m_machine; }

    void enable_interrupts() noexcept;
    void disable_interrupts() noexcept;
    bool ime() const noexcept { return m_intr_master_enable; }

    bool is_stopping() const noexcept { return m_stopped; }
    bool is_halting() const noexcept { return m_asleep || m_haltbug != 0; }

    // debugging
    void  breakpoint(uint16_t address, breakpoint_t func);
    auto& breakpoints() { return this->m_breakpoints; }
    void  default_pausepoint(uint16_t address);
    void  break_on_steps(int steps);
    void  break_now() { this->m_break = true; }
    void  break_checks();
    bool  is_breaking() const noexcept { return this->m_break; }
    static void print_and_pause(CPU&, const uint8_t opcode);

    std::string to_string() const;

  private:
    void handle_interrupts();
    void handle_speed_switch();
    void execute_interrupts(const uint8_t);
    bool break_time() const;

    regs_t   m_registers;
    Machine& m_machine;
    Memory&  m_memory;
    uint64_t m_cycles_total = 0;
    uint8_t  m_cur_opcode = 0xff;
    uint8_t  m_last_flags = 0xff;
    bool     m_intr_master_enable = false;
    int8_t   m_intr_pending = 0;
    bool     m_stopped = false;
    bool     m_asleep = false;
    uint8_t  m_switch_cycles = 0;
    uint8_t  m_haltbug = 0;
    // debugging
    bool     m_break = false;
    mutable int16_t  m_break_steps = 0;
    mutable int16_t  m_break_steps_cnt = 0;
    std::map<uint16_t, breakpoint_t> m_breakpoints;
  };

  inline void CPU::breakpoint(uint16_t addr, breakpoint_t func)
  {
    this->m_breakpoints[addr] = func;
  }

  inline void CPU::default_pausepoint(const uint16_t addr)
  {
    this->breakpoint(addr,
    breakpoint_t{
      [] (gbc::CPU& cpu, const uint8_t opcode) {
        print_and_pause(cpu, opcode);
      }
    });
  }
}
