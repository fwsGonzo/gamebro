#include "machine.hpp"
#include "bios.hpp"

namespace gbc
{
  Machine::Machine(const std::vector<uint8_t>& rom)
      : memory(*this), cpu(memory), io(*this),
        ddCharacter  {Memory::Display_Chr},
        ddBackground1{Memory::Display_BG1},
        ddBackground2{Memory::Display_BG2}
  {
    memory.program_area() = rom;
  }

  uint64_t Machine::now() noexcept
  {
    return cpu.gettime();
  }

  void Machine::break_now()
  {
    cpu.break_now();
  }

  void Machine::undefined()
  {
    if (this->stop_when_undefined) {
      printf("*** An undefined operation happened\n");
      cpu.break_now();
    }
  }
}
