#pragma once
#include <cstdint>

namespace gbc
{
  class CPU
  {
  public:
    CPU() = default;
    void simulate();
    void incr_cycles(int count);

  private:
    uint64_t m_cycles = 0;
  };
}
