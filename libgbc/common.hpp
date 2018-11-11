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
