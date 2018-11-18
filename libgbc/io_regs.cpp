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
    //printf("JOYP write 0x%02x\n", value);
    value &= 0x30;
    if      (value == 0x10) io.joypad().ioswitch = 0;
    else if (value == 0x20) io.joypad().ioswitch = 1;
    else printf("JOYP strange write: 0x%02x\n", value);
  }
  uint8_t ioread_JOYP(IO& io, uint16_t)
  {
    // logic
    switch (io.joypad().ioswitch) {
      //case 0: return 0xD0 | (~io.joypad().keypad  & 0xF);
      //case 1: return 0xE0 | (~io.joypad().buttons & 0xF);
      case 0: return 0xD0 | (~io.joypad().buttons & 0xF);
      case 1: return 0xE0 | (~io.joypad().keypad  & 0xF);
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

  void iowrite_STAT(IO& io, uint16_t addr, uint8_t value)
  {
    // can only write to the upper bits 3-7
    io.reg(addr) &= 0x7;
    io.reg(addr) |= value & 0xC;
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
    printf("HDMA 0x%04x write 0x%02x\n", addr, value);
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
    io.reg(addr) = 0x0;
    //assert(0 && "KEY1 registers written to");
  }
  uint8_t ioread_KEY1(IO& io, uint16_t addr)
  {
    return io.reg(addr);
  }

  void iowrite_VBK(IO& io, uint16_t addr, uint8_t value)
  {
    printf("VBK 0x%04x write 0x%02x\n", addr, value);
    io.reg(addr) = value & 1;
    io.machine().gpu.set_video_bank(value & 1);
  }
  uint8_t ioread_VBK(IO& io, uint16_t addr)
  {
    return io.reg(addr);
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
    IOHANDLER(IO::REG_DMA,   DMA);
    IOHANDLER(IO::REG_STAT,  STAT);
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
