#pragma once
#include <cstdint>
#include <util/delegate.hpp>

namespace gbc
{
  class CPU;

  struct breakpoint_t {
    delegate<void(CPU&, uint8_t)> callback;
  };

}
