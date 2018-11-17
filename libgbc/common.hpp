#pragma once

#ifndef LIKELY
#define LIKELY(x)       __builtin_expect((x),1)
#endif
#ifndef UNLIKELY
#define UNLIKELY(x)     __builtin_expect((x),0)
#endif

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
