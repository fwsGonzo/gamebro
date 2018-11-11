// should only be included once
#define IOHANDLER(off, x) new(&iologic.at(off-0xff00)) ioreg_t{ioreg_##x};

namespace gbc
{
  struct ioreg_t {
    using handler_t = void(*) (IO&, uint16_t, uint8_t);
    const handler_t handler = nullptr;
  };
  static std::array<ioreg_t, 128> iologic = {};

  void ioreg_DMA(IO& io, uint16_t addr, uint8_t value)
  {
    auto& memory = io.machine().memory;
    uint16_t src = value << 8;
    uint16_t dst = 0xfe00;
    printf("DMA copy from 0x%04x to 0x%04x\n", src, dst);
    for (int i = 0; i < 160; i++) {
      memory.write8(dst++, memory.read8(src++));
    }
    //assert(0 && "DMA operation completed");
  }
  void ioreg_HDMA(IO& io, uint16_t addr, uint8_t value)
  {
    printf("HDMA 0x%04x write 0x%02x\n", addr, value);
    assert(0 && "HDMA registers written to");
  }


  __attribute__((constructor))
  static void set_io_handlers() {
    IOHANDLER(IO::REG_DMA,   DMA);
    IOHANDLER(IO::REG_HDMA1, HDMA);
    IOHANDLER(IO::REG_HDMA2, HDMA);
    IOHANDLER(IO::REG_HDMA3, HDMA);
    IOHANDLER(IO::REG_HDMA4, HDMA);
    IOHANDLER(IO::REG_HDMA5, HDMA);
  }
}
