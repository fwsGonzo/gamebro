#include "machine.hpp"
#include "bios.hpp"

namespace gbc
{
  Machine::Machine(std::vector<uint8_t> rom)
      : memory(*this, std::move(rom)), cpu(memory), io(*this),
        ddCharacter  {Memory::Display_Chr},
        ddBackground1{Memory::Display_BG1},
        ddBackground2{Memory::Display_BG2}
  {
  }

  uint64_t Machine::now() noexcept
  {
    return cpu.gettime();
  }

  void Machine::break_now()
  {
    cpu.break_now();
  }
  bool Machine::is_breaking() const noexcept
  {
    return cpu.is_breaking();
  }

  void Machine::undefined()
  {
    if (this->stop_when_undefined) {
      printf("*** An undefined operation happened\n");
      cpu.break_now();
    }
  }
}
