#include "io.hpp"
#include <cstdio>
#include "machine.hpp"
#include "io_regs.cpp"

namespace gbc
{
  IO::IO(Machine& mach)
      : vblank   {  0x1, 0x40, "V-blank" },
        lcd_stat {  0x2, 0x48, "LCD Status" },
        timer    {  0x4, 0x50, "Timer" },
        serial   {  0x8, 0x58, "Serial" },
        joypad   { 0x10, 0x60, "Joypad" },
        m_machine(mach)
  {
    this->reset();
  }

  void IO::reset()
  {
    // register defaults
    reg(REG_LCDC) = 0x91;
    reg(REG_STAT) = 0x85;
    reg(REG_DMA)  = 0xff;
    reg(REG_BGP)  = 0xfc;
    reg(REG_TAC)  = 0xf8;
    reg(REG_IF)   = 0xe1;
    reg(REG_BOOT) = 0x01;
    this->m_reg_ie = 0x00;
  }

  void IO::simulate()
  {
    const uint64_t t = machine().now();
    // check if LCD is operating
    if (reg(REG_LCDC) & 0x80)
    {
      /*
      printf("LCDC = 0x%02x\n", reg(REG_LCDC));
      printf("STAT = 0x%02x\n", reg(REG_STAT));
      printf("IF = 0x%02x\n", reg(REG_IF));
      machine().break_now();
      */

      static const int VBLANK_CYCLES = 4560;
      // vblank always when screen on
      if (t >= vblank.last_time + VBLANK_CYCLES)
      {
        vblank.last_time = t;
        // scanline LY increment logic
        const int MAX_LINES = 153;
        m_ly = (m_ly + 1) % MAX_LINES;
        reg(REG_LY) = m_ly;

        if (is_vblank()) this->trigger(vblank);
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

  uint8_t IO::read_io(const uint16_t addr)
  {
    // default: just return the register value
    if (addr >= 0xff00 && addr < 0xff4c) {
      if (machine().break_on_io && !machine().is_breaking()) {
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
    return 0xff;
  }
  void IO::write_io(const uint16_t addr, uint8_t value)
  {
    // default: just write to register
    if (addr >= 0xff00 && addr < 0xff4c) {
      if (machine().break_on_io && !machine().is_breaking()) {
        printf("[io] * I/O write 0x%04x value 0x%02x\n", addr, value);
        machine().break_now();
      }
      auto& handler = iologic.at(addr - 0xff00);
      if (handler.handler != nullptr) {
        handler.handler(*this, addr, value);
        return;
      }
      // default: just write...
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
  bool IO::is_vblank()
  {
    return (this->m_ly >= 144);
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
    // sometimes we want to break on interrupts
    if (machine().break_on_interrupts && !machine().is_breaking()) {
      machine().break_now();
    }
    if (intr.callback) intr.callback(machine(), intr);
  }
  uint8_t IO::interrupt_mask() {
    return this->reg(REG_IF);
  }
}
