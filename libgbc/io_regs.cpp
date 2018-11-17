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

  void iowrite_JOYP(IO& io, uint16_t addr, uint8_t value)
  {
    // TODO: joypad functionality
    printf("P1/JOYP register 0x%04x write 0x%02x\n", addr, value);
  }
  uint8_t ioread_JOYP(IO& io, uint16_t)
  {
    // logic
    return 0x0;
  }

  void iowrite_DIV(IO& io, uint16_t, uint8_t)
  {
    // writing to DIV resets it to 0
    io.reg(IO::REG_DIV) = 0;
    io.machine().break_now();
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

  void iowrite_DMA(IO& io, uint16_t addr, uint8_t value)
  {
    const uint16_t src = value << 8;
    //const uint16_t dst = 0xfe00;
    //printf("DMA copy start from 0x%04x to 0x%04x\n", src, dst);
    io.start_dma(src);
  }
  uint8_t ioread_DMA(IO& io, uint16_t addr)
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

  void iowrite_HDMA(IO& io, uint16_t addr, uint8_t value)
  {
    printf("HDMA 0x%04x write 0x%02x\n", addr, value);
    assert(0 && "HDMA registers written to");
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

  __attribute__((constructor))
  static void set_io_handlers() {
    IOHANDLER(IO::REG_P1,    JOYP);
    IOHANDLER(IO::REG_DIV,   DIV);
    IOHANDLER(IO::REG_DMA,   DMA);
    IOHANDLER(IO::REG_STAT,  STAT);
    IOHANDLER(IO::REG_KEY1,  KEY1);
    IOHANDLER(IO::REG_HDMA1, HDMA);
    IOHANDLER(IO::REG_HDMA2, HDMA);
    IOHANDLER(IO::REG_HDMA3, HDMA);
    IOHANDLER(IO::REG_HDMA4, HDMA);
    IOHANDLER(IO::REG_HDMA5, HDMA);
    IOHANDLER(IO::REG_NR52,  SND_ONOFF);
    IOHANDLER(IO::REG_BOOT,  BOOT);
  }
}
