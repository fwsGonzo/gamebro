#pragma once
#include <cstdint>

enum reg_flags_t {
  MASK_ZERO      = 0x80,
  MASK_NEGATIVE  = 0x40,
  MASK_HALFCARRY = 0x20,
  MASK_CARRY     = 0x10,
};

struct regs_t
{
  void write_flags(uint8_t value) {
    this->f = value & 0xF0;
  }

  union {
    struct {
      uint8_t f;
      uint8_t a;
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
};
