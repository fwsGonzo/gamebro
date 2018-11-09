#pragma once
#include <cstdint>
#include <string>

namespace gbc
{
  enum flags_t {
    MASK_ZERO      = 0x80,
    MASK_NEGATIVE  = 0x40,
    MASK_HALFCARRY = 0x20,
    MASK_CARRY     = 0x10,
  };

  struct regs_t
  {
    union {
      struct {
        uint8_t flags;
        uint8_t accum;
      };
      uint16_t af;
    };
    union {
      struct {
        uint8_t c;
        uint8_t b;
      };
      uint16_t bc;
    };
    union {
      struct {
        uint8_t e;
        uint8_t d;
      };
      uint16_t de;
    };
    union {
      struct {
        uint8_t l;
        uint8_t h;
      };
      uint16_t hl;
    };

    uint16_t sp;
    uint16_t pc;

    inline uint16_t& getreg(const uint8_t bf, const bool use_sp) {
      switch (bf & 0x3) {
      case 0: return bc;
      case 1: return de;
      case 2: return hl;
      case 3: return (use_sp) ? sp : af;
      }
      __builtin_unreachable();
    }
    uint16_t& getreg_sp(const uint8_t opcode) {
      return getreg((opcode >> 4) & 0x3, true);
    }
    uint16_t& getreg_af(const uint8_t opcode) {
      return getreg((opcode >> 4) & 0x3, false);
    }

    uint8_t& getdest(const uint8_t bf) {
      switch (bf & 0x7) {
      case 0: return b;
      case 1: return c;
      case 2: return d;
      case 3: return e;
      case 4: return h;
      case 5: return l;
      case 6: throw std::runtime_error("getdest: (HL) not accessible here");
      case 7: return accum;
      }
      __builtin_unreachable();
    }

    uint8_t& getextr(const uint8_t bf) {
      switch (bf & 0x7) {
      case 0: return accum;
      case 1: return b;
      case 2: return c;
      case 3: return d;
      case 4: return e;
      case 5: return h;
      case 6: return l;
      case 7: throw std::runtime_error("getextr: (HL) not accessible here");
      }
      __builtin_unreachable();
    }

    bool compare_flags(const uint8_t opcode) noexcept {
      const uint8_t idx = (opcode >> 3) & 0x3;
      if (idx == 0) return (flags & MASK_ZERO) == 0; // not zero
      if (idx == 1) return (flags & MASK_ZERO); // zero
      if (idx == 2) return (flags & MASK_CARRY) == 0; // not carry
      if (idx == 3) return (flags & MASK_CARRY); // carry
      __builtin_unreachable();
    }

    void alu(uint8_t op, uint8_t value) noexcept
    {
      auto& reg = this->accum;
      switch (op & 0x7) {
        case 0x0: // ADD
            reg += value;
            if (value > reg) flags |= MASK_CARRY;
            if ((value & 0xF) > (reg & 0xF)) flags |= MASK_HALFCARRY;
            return;
        case 0x1: // ADC
            reg += value;
            if (flags & MASK_CARRY) reg++;
            if (value > reg) flags |= MASK_CARRY;
            if ((value & 0xF) > (reg & 0xF)) flags |= MASK_HALFCARRY;
            return;
        case 0x2: // SUB
            reg -= value;
            if (value > reg) flags |= MASK_CARRY;
            if ((value & 0xF) > (reg & 0xF)) flags |= MASK_HALFCARRY;
            return;
        case 0x3: // SBC
            reg -= value;
            if (flags & MASK_CARRY) reg--;
            if (value > reg) flags |= MASK_CARRY;
            if ((value & 0xF) > (reg & 0xF)) flags |= MASK_HALFCARRY;
            return;
        case 0x4: // AND
            reg &= value;
            flags = 0;
            if (reg == 0) flags |= MASK_ZERO;
            flags |= MASK_HALFCARRY;
            return;
        case 0x5: // XOR
            reg ^= value;
            flags = 0;
            if (reg == 0) flags |= MASK_ZERO;
            return;
        case 0x6: // OR
            reg |= value;
            flags = 0;
            if (reg == 0) flags |= MASK_ZERO;
            return;
        case 0x7:
            //printf("\nCP 0x%02x vs 0x%02x (HL=0x%04x)\n", reg, value, hl);
            const uint8_t tmp = reg - value;
            flags = MASK_NEGATIVE;
            if (reg == value) flags |= MASK_ZERO;
            if (value > tmp) flags |= MASK_CARRY;
            return;
      }
    } // alu()

    std::string to_string() const {
      char buffer[512];
      int len = snprintf(buffer, sizeof(buffer),
          "\tAF = 0x%04x  BC = 0x%04x  A = 0x%02x  F = 0x%02x  B = 0x%02x\n"
          "\tDE = 0x%04x  HL = 0x%04x  C = 0x%02x  D = 0x%02x  E = 0x%02x\n"
          "\tSP = 0x%04x  PC = 0x%04x  H = 0x%02x  L = 0x%02x\n",
          af, bc, accum, flags, b,
          de, hl, c, d, e,
          sp, pc, h, l);
      return std::string(buffer, len);
    }
  };

  inline flags_t to_flag(const uint8_t opcode) {
    return (flags_t) (1 >> (4 + ((opcode >> 3) & 0x3)));
  }
}
