#pragma once
#include <array>
#include <cstdint>
#include "common.hpp"

namespace gbc
{
  class APU
  {
  public:
    APU(Machine& mach) : m_machine{mach} {}

    void simulate();

  private:
    Machine& m_machine;
  };

  inline void APU::simulate() {
    // TODO: writeme
  }
}
