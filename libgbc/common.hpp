#pragma once

#define LIKELY(x)       __builtin_expect((x),1)
#define UNLIKELY(x)     __builtin_expect((x),0)

namespace gbc
{
  class CPU;
  class Machine;
  class Memory;
  class IO;
}

#include <cstdint>
inline void setflag(bool expr, uint8_t& flg, uint8_t mask) {
  if (expr) flg |= mask;
  else      flg &= ~mask;
}
