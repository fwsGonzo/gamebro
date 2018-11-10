#include "io.hpp"
#include <cstdio>
#include "machine.hpp"

namespace gbc
{

  IO::IO(Machine& mach)
      : vblank   {  0x1, 0x40, "V-blank" },
        lcd_stat {  0x2, 0x48, "LCD Status" },
        timer    {  0x4, 0x50, "Timer" },
        serial   {  0x8, 0x58, "Serial" },
        joypad   { 0x10, 0x60, "Joypad" },
        m_machine(mach)
  {}

  void IO::simulate()
  {
    const uint64_t t = machine().now();
    // check if LCD is operating
    if (reg(REG_LCDC) & 0x80)
    {
      printf("IME ENABLED, LCDC = 0x%02x\n", reg(REG_LCDC));
      printf("STAT = 0x%02x\n", reg(REG_STAT));
      printf("IF = 0x%02x\n", reg(REG_IF));
      machine().break_now();

      // vblank always when screen on
      if (t >= vblank.last_time + 70224) {
        vblank.last_time = t;
        this->interrupt(vblank);
      }

      if (reg(REG_STAT) & 0x3) {
        // LCD status?
      }
    }
    if (reg(REG_TAC) & 0x4)
    {
      printf("TAC = 0x%02x\n", reg(REG_TAC));
      machine().break_now();

    }
  }

  static const bool BREAK_ON_IO = true;

  uint8_t IO::read_io(const uint16_t addr)
  {
    // default: just return the register value
    if (addr >= 0xff00 && addr < 0xff4c) {
      if (BREAK_ON_IO && machine().cpu.is_breaking() == false) {
        printf("[io] * I/O read 0x%04x => 0x%02x\n", addr, reg(addr));
        machine().break_now();
      }
      return reg(addr);
    }
    if (addr == REG_IE) {
      return m_reg_ie;
    }
    printf("[io] * Unknown read 0x%04x\n", addr);
    machine().undefined();
    return 0;
  }
  void IO::write_io(const uint16_t addr, uint8_t value)
  {
    // default: just write to register
    if (addr >= 0xff00 && addr < 0xff4c) {
      if (BREAK_ON_IO && machine().cpu.is_breaking() == false) {
        printf("[io] * I/O write 0x%04x value 0x%02x\n", addr, value);
        machine().break_now();
      }
      reg(addr) = value;
      return;
    }
    if (addr == REG_IE) {
      m_reg_ie = value;
      return;
    }
    printf("[io] * Unknown write 0x%04x value 0x%02x\n", addr, value);
    machine().undefined();
  }

  void IO::trigger_key(key_t key)
  {
    reg(REG_P1) |= key;
  }

  void IO::trigger(interrupt_t& intr)
  {
    reg(REG_IF) |= intr.mask;
  }
  void IO::interrupt(interrupt_t& intr)
  {
    printf("Executing interrupt %s (%#x)\n", intr.name, intr.mask);
    unsigned int t = 12;
    // disable interrupt
    reg(REG_IF) &= ~intr.mask;
    // push PC and jump to INTR addr
    t += machine().cpu.push_and_jump(intr.fixed_address);
    machine().cpu.incr_cycles(t);
    machine().break_now();
  }
  uint8_t IO::interrupt_mask() {
    return this->reg(REG_IF);
  }
}
