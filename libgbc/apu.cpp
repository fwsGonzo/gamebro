#include "apu.hpp"
#include "machine.hpp"
#include "io.hpp"

namespace gbc
{
  APU::APU(Machine& mach)
   : m_machine{mach}
  {

  }

  void APU::simulate()
  {
    // if sound is off, don't do anything
    if ((machine().io.reg(IO::REG_NR52) & 0x80) == 0) return;

    // TODO: writeme
    
  }

}
