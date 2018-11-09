#include "io.hpp"
#include <cstdio>
#include "machine.hpp"

namespace gbc
{
  IO::IO(Machine& mach)
      : m_machine(mach)
  {

  }

  uint8_t IO::read_io(const uint16_t addr)
  {
    switch (addr) {
      case 0xff00:
          printf("[io] Reading P1 (0x%02x)\n", m_p1);
          return m_p1;
      case 0xffff:
          printf("[io] Reading Interrupts (0x%02x)\n", m_interrupts);
          return m_interrupts;
    }
    printf("[io] * Unknown read 0x%04x\n", addr);
    machine().undefined();
    return 0;
  }
  void IO::write_io(const uint16_t addr, uint8_t value)
  {
    switch (addr) {
      case 0xff00:
          printf("[io] Write to P1 (0x%02x)\n", value);
          m_p1 = value;
          return;
      case 0xffff:
          printf("[io] Writing Interrupts (0x%02x)\n", value);
          m_interrupts = value;
          return;
    }
    printf("[io] * Unknown write 0x%04x value 0x%02x\n", addr, value);
    machine().undefined();
  }
}
