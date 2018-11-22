// should only be included once
#define IOHANDLER(off, x) new(&iologic.at(off-0xff00)) iowrite_t{iowrite_##x, ioread_##x};

namespace gbc
{
  struct iowrite_t {
    using write_handler_t = void(*) (IO&, uint16_t, uint8_t);
    using read_handler_t  = uint8_t(*) (IO&, uint16_t);
    const write_handler_t on_write = nullptr;
    const read_handler_t  on_read  = nullptr;
  };
  static std::array<iowrite_t, 128> iologic = {};

  void iowrite_JOYP(IO& io, uint16_t, uint8_t value)
  {
    if      (value & 0x10) io.joypad().ioswitch = 0;
    else if (value & 0x20) io.joypad().ioswitch = 1;
  }
  uint8_t ioread_JOYP(IO& io, uint16_t)
  {
    switch (io.joypad().ioswitch) {
      case 0: return 0xD0 | io.joypad().buttons;
      case 1: return 0xE0 | io.joypad().keypad;
    }
    assert(0 && "Invalid joypad GPIO value");
  }

  void iowrite_DIV(IO& io, uint16_t, uint8_t)
  {
    // writing to DIV resets it to 0
    io.reg(IO::REG_DIV) = 0;
  }
  uint8_t ioread_DIV(IO& io, uint16_t)
  {
    return io.reg(IO::REG_DIV);
  }

  void iowrite_LCDC(IO& io, uint16_t addr, uint8_t value)
  {
    const bool was_enabled = io.reg(addr) & 0x80;
    io.reg(addr) = value;
    const bool is_enabled = io.reg(addr) & 0x80;
    // check if LCD just turned on
    if (!was_enabled && is_enabled)
    {
      io.machine().gpu.lcd_power_changed(true);
    }
    // check if LCD was just turned off
    else if (was_enabled && !is_enabled)
    {
      io.machine().gpu.lcd_power_changed(false);
    }
  }
  uint8_t ioread_LCDC(IO& io, uint16_t addr)
  {
    return io.reg(addr);
  }

  void iowrite_STAT(IO& io, uint16_t addr, uint8_t value)
  {
    // can only write to the upper bits 3-6
    io.reg(addr) &= 0x87;
    io.reg(addr) |= value & 0x78;
  }
  uint8_t ioread_STAT(IO& io, uint16_t addr)
  {
    return io.reg(addr);
  }

  void iowrite_DMA(IO& io, uint16_t, uint8_t value)
  {
    const uint16_t src = value << 8;
    //printf("DMA copy start from 0x%04x to 0x%04x\n", src, dst);
    io.start_dma(src);
  }
  uint8_t ioread_DMA(IO& io, uint16_t addr)
  {
    return io.reg(addr);
  }

  void iowrite_HDMA(IO& io, uint16_t addr, uint8_t value)
  {
    switch (addr) {
      case IO::REG_HDMA1:
      case IO::REG_HDMA3:
          io.reg(addr) = value;
          return;
      case IO::REG_HDMA2:
      case IO::REG_HDMA4:
          io.reg(addr) = value & 0xF0;
          return;
    }
    // HDMA 5: start DMA operation
    uint16_t src = (io.reg(IO::REG_HDMA1) << 8) | io.reg(IO::REG_HDMA2);
    src &= 0xFFF0;
    uint16_t dst = (io.reg(IO::REG_HDMA3) << 8) | io.reg(IO::REG_HDMA4);
    dst &= 0x9FF0;
    uint16_t end = src + (io.reg(IO::REG_HDMA5) & 0x7F) * 16;
    // do the transfer immediately
    printf("HDMA transfer 0x%04x to 0x%04x (%u bytes)\n", src, dst, end - src);
    auto& mem = io.machine().memory;
    while (src < end) mem.write8(dst++, mem.read8(src++));
    // transfer complete
    io.reg(IO::REG_HDMA5) = 0xFF;
  }
  uint8_t ioread_HDMA(IO& io, uint16_t addr)
  {
    return io.reg(addr);
  }

  void iowrite_SND_ONOFF(IO& io, uint16_t addr, uint8_t value)
  {
    // TODO: writing bit7 should clear all sound registers
    printf("NR52 Sound ON/OFF 0x%04x write 0x%02x\n", addr, value);
    io.reg(IO::REG_NR52) &= 0xF;
    io.reg(IO::REG_NR52) |= value & 0x80;
    //assert(0 && "NR52 Sound ON/OFF register write");
  }
  uint8_t ioread_SND_ONOFF(IO& io, uint16_t addr)
  {
    return io.reg(addr);
  }

  void iowrite_KEY1(IO& io, uint16_t addr, uint8_t value)
  {
    printf("KEY1 0x%04x write 0x%02x\n", addr, value);
    io.reg(addr) &= 0x80;
    io.reg(addr) |= value & 1;
  }
  uint8_t ioread_KEY1(IO& io, uint16_t addr)
  {
    return io.reg(addr) | (io.machine().is_cgb() ? 0x7E : 0xFF);
  }

  void iowrite_VBK(IO& io, uint16_t addr, uint8_t value)
  {
    printf("VBK 0x%04x write 0x%02x\n", addr, value);
    io.reg(addr) = value & 1;
    io.machine().gpu.set_video_bank(value & 1);
  }
  uint8_t ioread_VBK(IO& io, uint16_t addr)
  {
    return io.reg(addr) | 0xfe;
  }

  void iowrite_BOOT(IO& io, uint16_t addr, uint8_t value)
  {
    if (value) {
      io.machine().memory.disable_bootrom();
    }
    io.reg(addr) |= value;
  }
  uint8_t ioread_BOOT(IO& io, uint16_t addr)
  {
    return io.reg(addr);
  }

  void iowrite_SVBK(IO& io, uint16_t addr, uint8_t value)
  {
    printf("SVBK 0x%04x write 0x%02x\n", addr, value);
    io.reg(addr) = value & 1;
    io.machine().memory.set_wram_bank(value & 1);
  }
  uint8_t ioread_SVBK(IO& io, uint16_t addr)
  {
    return io.reg(addr);
  }

  __attribute__((constructor))
  static void set_io_handlers() {
    IOHANDLER(IO::REG_P1,    JOYP);
    IOHANDLER(IO::REG_DIV,   DIV);
    IOHANDLER(IO::REG_LCDC,  LCDC);
    IOHANDLER(IO::REG_STAT,  STAT);
    IOHANDLER(IO::REG_DMA,   DMA);
    IOHANDLER(IO::REG_KEY1,  KEY1);
    IOHANDLER(IO::REG_VBK,   VBK);
    IOHANDLER(IO::REG_HDMA1, HDMA);
    IOHANDLER(IO::REG_HDMA2, HDMA);
    IOHANDLER(IO::REG_HDMA3, HDMA);
    IOHANDLER(IO::REG_HDMA4, HDMA);
    IOHANDLER(IO::REG_HDMA5, HDMA);
    IOHANDLER(IO::REG_NR52,  SND_ONOFF);
    IOHANDLER(IO::REG_BOOT,  BOOT);
  }
}
