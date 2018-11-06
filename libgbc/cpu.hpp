#pragma once
#include <array>
#include <cstdint>
#include <util/delegate.hpp>

namespace gbc
{
  class Machine;

  enum reg_flags_t {
    MASK_ZERO       = 0x80,
    MASK_OPERATION  = 0x40,
    MASK_HALF_CARRY = 0x20,
    MASK_CARRY      = 0x10,
  };

  class CPU
  {
  public:
    using opcode_t = delegate<void(CPU&, uint8_t)>;
    enum regs_t {
      REG_A = 0,
      REG_B,
      REG_C,
      REG_D,
      REG_E,
      REG_H,
      REG_L,
      REG_FLAGS,
      REG_M,
      REG_T,
      REGS
    };

    CPU(Machine&) noexcept;
    void  reset() noexcept;
    void  simulate();
    void  execute(uint8_t opcode);
    void  incr_cycles(int count);

    uint8_t readop(int dx) const;

    void reg_set(const regs_t reg, uint8_t value) {
      m_regs.at((int) reg) = value;
    }
    uint8_t reg_read(const regs_t reg) const {
      return m_regs.at((int) reg);
    }

    void sp_set(uint16_t value) noexcept {
      this->m_sp = value;
    }
    uint16_t sp_read() const noexcept {
      return this->m_sp;
    }

    void pc_set(uint16_t value) noexcept {
      this->m_pc = value;
    }
    uint16_t pc_read() const noexcept {
      return this->m_pc;
    }

  private:
    Machine& m_machine;
    std::array<uint8_t, REGS> m_regs = {0};
    uint16_t m_sp = 0x0;
    uint16_t m_pc = 0x0;
    uint64_t m_cycles_total = 0;

    std::array<opcode_t, 256> ops;
  };
  using opcode_t = CPU::opcode_t;
}
