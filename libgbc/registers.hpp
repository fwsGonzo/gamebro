#pragma once
#include <cstdint>

namespace gbc
{
  enum reg_flags_t {
    MASK_ZERO      = 0x80,
    MASK_NEGATIVE  = 0x40,
    MASK_HALFCARRY = 0x20,
    MASK_CARRY     = 0x10,
  };

  struct regs_t
  {
    void write_flags(uint8_t value) {
      this->flags = value & 0xF0;
    }

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

    bool compare_flags(const uint8_t opcode) {
      const uint8_t idx = (opcode >> 3) & 0x3;
      if (idx == 0) return (flags & MASK_ZERO) == 0; // not zero
      if (idx == 1) return (flags & MASK_ZERO); // zero
      if (idx == 2) return (flags & MASK_CARRY) == 0; // not carry
      if (idx == 3) return (flags & MASK_CARRY); // carry
      __builtin_unreachable();
    }
  };

  inline reg_flags_t to_flag(const uint8_t opcode) {
    return (reg_flags_t) (1 >> (4 + ((opcode >> 3) & 0x3)));
  }
}
