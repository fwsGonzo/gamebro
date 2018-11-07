#pragma once
#include <util/delegate.hpp>

namespace gbc
{
  class  CPU;
  struct instruction_t;
  using handler_t = delegate<unsigned(CPU&, const instruction_t&)>;

  struct instruction_t {
    const std::string mnemonic;
    const uint16_t    length;
    handler_t         handler;
  };
}
