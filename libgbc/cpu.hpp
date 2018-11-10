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
  class Machine;
  class Memory;

  class CPU
  {
  public:
    using breakpoint_t = delegate<void(CPU&, uint8_t)>;
    CPU(Memory&) noexcept;
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
    void     wait();
    instruction_t& resolve_instruction(uint8_t opcode);

    regs_t& registers() noexcept { return m_registers; }

    Memory&  memory() noexcept { return m_memory; }
    Machine& machine() noexcept { return m_machine; }

    void enable_interrupts() noexcept { m_intr_master_enable = true; }
    void disable_interrupts() noexcept { m_intr_master_enable = false; }
    bool ime() const noexcept { return m_intr_master_enable; }

    bool is_running() const noexcept { return m_running; }
    bool is_waiting() const noexcept { return m_waiting; }

    // debugging
    void breakpoint(uint16_t address, breakpoint_t func);
    void default_pausepoint(uint16_t address, bool single_step);
    void single_step(bool en) { m_singlestep = en; }
    void break_now() { this->m_break = true; }
    bool is_breaking() const noexcept { return this->m_break; }
    static void print_and_pause(CPU&, const uint8_t opcode);

    std::string to_string() const;

  private:
    void execute_interrupts(const uint8_t);
    regs_t   m_registers;
    Memory&  m_memory;
    Machine& m_machine;
    uint64_t m_cycles_total = 0;
    uint8_t  m_cur_opcode = 0xff;
    uint8_t  m_last_flags = 0xff;
    bool     m_intr_master_enable = false;
    bool     m_running = true;
    bool     m_waiting = false;
    bool     m_singlestep = false;
    bool     m_break = false;
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
