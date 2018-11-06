#include "cpu.hpp"
#include <cassert>

namespace gbc
{
  void CPU::simulate()
  {
    asm("pause");
    this->incr_cycles(1);
  }

  void CPU::incr_cycles(int count)
  {
    assert(count >= 0);
    this->m_cycles += count;
  }

}
