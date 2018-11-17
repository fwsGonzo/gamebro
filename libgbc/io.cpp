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
        debug    {  0x0,  0x0, "Debug" },
        m_machine(mach)
  {
    this->reset();
  }

  void IO::reset()
  {
    // register defaults
    reg(REG_P1)   = 0xcf;
    reg(REG_TIMA) = 0x00;
    reg(REG_TMA)  = 0x00;
    reg(REG_TAC)  = 0xf8;
    // sound defaults
    reg(REG_NR10)   = 0x80;
    reg(REG_NR11)   = 0xbf;
    reg(REG_NR52)   = 0xf1;
    // LCD defaults
    reg(REG_LCDC) = 0x91;
    reg(REG_STAT) = 0x81;
    reg(REG_LY)   = 0x90; // 144
    reg(REG_LYC)  = 0x0;
    reg(REG_DMA)  = 0x00;
    // Palette
    reg(REG_BGP)  = 0xfc;
    reg(REG_OBP0) = 0xff;
    reg(REG_OBP1) = 0xff;
    // boot rom enabled at boot
    reg(REG_BOOT) = 0x00;

    this->m_reg_ie = 0x00;
  }

  void IO::simulate()
  {
    const uint64_t t = machine().now();
    // 1. timers
    static const int DIV_CYCLES = 256;
    // divider timer
    if (t >= this->m_divider_time + DIV_CYCLES)
    {
      this->m_divider_time = t;
      this->reg(REG_DIV)++;
    }
    // check if timer is enabled
    if (this->reg(REG_TAC) & 0x4)
    {
      const int TIMA_CYCLES[] = {1024, 16, 64, 256};
      const int speed = this->reg(REG_TAC) & 0x3;
      // TIMA counter timer
      if (t >= timer.last_time + TIMA_CYCLES[speed])
      {
        timer.last_time = t;
        this->reg(REG_TIMA)++;
        // timer interrupt when overflowing to 0
        if (this->reg(REG_TIMA) == 0) {
          this->trigger(timer);
          // restart at modulo
          this->reg(REG_TIMA) = this->reg(REG_TMA);
        }
      }
    }

    // check if LCD is operating
    if (reg(REG_LCDC) & 0x80)
    {
      // 2. LCD
      static const int SCANLINE_CYCLES = 456;
      // vblank always when screen on
      if (t >= vblank.last_time + SCANLINE_CYCLES)
      {
        vblank.last_time = t;
        // scanline LY increment logic
        const int MAX_LINES = 153;
        m_ly = (m_ly + 1) % MAX_LINES;
        reg(REG_LY) = m_ly;

        if (this->m_ly == 144) {
          assert(is_vblank());
          // vblank interrupt
          this->trigger(vblank);
          // modify stat
          reg(REG_STAT) &= 0xfc;
          reg(REG_STAT) |= 0x1;
          // if STAT vblank interrupt is enabled
          if (reg(REG_STAT) & 0x10) {
            this->trigger(lcd_stat);
          }
        }
        else if (this->m_ly == 0) {
          m_vblank_end = t;
          // set mode to 0
          this->m_scanmode = 0;
        }
        // STAT coincidence bit
        if (this->m_ly == reg(REG_LYC)) {
          reg(REG_STAT) |= 0x4;
          // STAT interrupt (if enabled)
          if (reg(REG_STAT) & 0x40) {
            this->trigger(lcd_stat);
          }
        }
        else {
          reg(REG_STAT) &= ~0x4;
        }
      }
      // STAT mode & scanline period modulation
      if (is_vblank() == false)
      {
        const uint64_t period = t - m_vblank_end;
        if (m_scanmode == 0) // period < 80
        {
          m_scanmode = 1;
          // if we are setting MODE 2 now
          if ((reg(REG_STAT) & 0x2) != 0x2) {
            // check if need to interrupt
            if (reg(REG_STAT) & 0x20) this->trigger(lcd_stat);
            reg(REG_STAT) &= 0xfc;
            reg(REG_STAT) |= 0x2;
          }
        }
        else if (m_scanmode == 1 && period >= 178)
        {
          m_scanmode = 2;
          reg(REG_STAT) &= 0xfc; // MODE 3
          reg(REG_STAT) |= 0x3;
        }
        else if (m_scanmode == 2 && period >= 204)
        {
          m_scanmode = 3; // H-blank
          // if we are setting MODE 0 now
          if (reg(REG_STAT) & 0x3) {
            // check if need to interrupt
            if (reg(REG_STAT) & 0x8) this->trigger(lcd_stat);
            reg(REG_STAT) &= 0xfc;
            reg(REG_STAT) |= 0x0;
          }
        }
        //printf("Mode: %#x  LY: %u\n", reg(REG_STAT), m_ly);
      }
    }

    // DMA operation
    if (this->m_dma.bytes_left > 0)
    {
      uint64_t tnow = machine().cpu.gettime();
      uint64_t diff = tnow - m_dma.cur_time;
      m_dma.cur_time = tnow;
      // calculate number of bytes to copy
      int btw = diff / 4;
      if (btw > m_dma.bytes_left) btw = m_dma.bytes_left;
      // do the copying
      auto& memory = machine().memory;
      for (int i = 0; i < btw; i++) {
        memory.write8(m_dma.dst++, memory.read8(m_dma.src++));
      }
      assert(m_dma.bytes_left >= btw);
      m_dma.bytes_left -= btw;
    }
  }

  uint8_t IO::read_io(const uint16_t addr)
  {
    // default: just return the register value
    if (addr >= 0xff00 && addr < 0xff80) {
      if (machine().break_on_io && !machine().is_breaking()) {
        printf("[io] * I/O read 0x%04x => 0x%02x\n", addr, reg(addr));
        machine().break_now();
      }
      auto& handler = iologic.at(addr - 0xff00);
      if (handler.on_read != nullptr) {
        return handler.on_read(*this, addr);
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
    if (addr >= 0xff00 && addr < 0xff80) {
      if (machine().break_on_io && !machine().is_breaking()) {
        printf("[io] * I/O write 0x%04x value 0x%02x\n", addr, value);
        machine().break_now();
      }
      auto& handler = iologic.at(addr - 0xff00);
      if (handler.on_write != nullptr) {
        handler.on_write(*this, addr, value);
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
    this->reg(REG_IF) |= intr.mask;
    //printf("Triggering interrupt: 0x%02x => 0x%02x\n", intr.mask, reg(REG_IF));
  }
  void IO::interrupt(interrupt_t& intr)
  {
    if (machine().verbose_interrupts) {
      printf("%9lu: Executing interrupt %s (%#x)\n",
             machine().cpu.gettime(), intr.name, intr.mask);
    }
    unsigned int t = 12;
    // disable interrupt request
    reg(REG_IF) &= ~intr.mask;
    // set interrupt bit
    this->m_reg_ie |= intr.mask;
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

  void IO::start_dma(uint16_t src)
  {
    m_dma.cur_time = machine().cpu.gettime();
    m_dma.end_time = m_dma.cur_time + 1280;
    m_dma.src = src;
    m_dma.dst = 0xfe00;
    m_dma.bytes_left = 160; // 160 bytes total
  }
}
