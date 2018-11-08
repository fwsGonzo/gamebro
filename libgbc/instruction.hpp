#pragma once
#include <util/delegate.hpp>
#include <string>

namespace gbc
{
  class  CPU;
  struct instruction_t;
  using handler_t = delegate<unsigned(CPU&, uint8_t)>;
  using printer_t = delegate<int(char*, size_t, CPU&, uint8_t)>;

  enum alu_t : uint8_t {
    ADD = 0,
    ADC,
    SUB,
    SBC,
    AND,
    XOR,
    OR,
    CP
  };

  enum opertype_t : uint8_t {
    NOTHING,
    RST_VECTOR,
    ADDRESS16,
    ADDR_FLAGS,
    REG_IMM8,
    REG_BC_DE,
    REG_NOT_SP,
    REG_NOT_AF,
    REG_IMM_SP,
    REG_IMM_AF,
    DESTINATION,
    IMMEDIATE,
  };

  struct instruction_t {
    handler_t         handler;
    printer_t         printer;
  };
}
