#include "machine.hpp"
#include "bios.hpp"

namespace gbc
{
  Machine::Machine(const std::vector<uint8_t>& rom)
      : memory(*this), cpu(memory),
        ddCharacter  {Memory::Display_Chr},
        ddBackground1{Memory::Display_BG1},
        ddBackground2{Memory::Display_BG2}
  {
    // install ROM at 0x100
    size_t size = std::min(rom.size(),
                  memory.program_area().size());
    std::copy(rom.begin(), rom.begin() + size,
              memory.program_area().begin());
  }

}
