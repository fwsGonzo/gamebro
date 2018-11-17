///////////////////////////////////
// Gamebro Emulator as a Service //
///////////////////////////////////
#include <service>
#include <timers>
#include <memdisk>
#include <hw/vga_gfx.hpp>
#include <hw/ps2.hpp>

#include "backbuffer.cpp"
#include <machine.hpp>
void Service::start()
{
  VGA_gfx::set_mode(VGA_gfx::MODE_320_200_256);
  VGA_gfx::clear();
  VGA_gfx::apply_default_palette();
  clear();

  fs::memdisk().init_fs(
  [] (fs::error_t err, fs::File_system&) {
    assert(!err);
  });
  auto& filesys = fs::memdisk().fs();
  auto rombuffer = filesys.read_file("/tloz_la.gb");
  auto romdata = std::move(*rombuffer.get());

  static gbc::Machine* machine = nullptr;
  machine = new gbc::Machine(romdata);
  machine->gpu.set_pixelmode(gbc::PM_PALETTE);

  // framebuffer
  using namespace std::chrono;
  Timers::periodic(16ms,
  [] (int) {
    static bool vblanked = false;
    machine->set_handler(gbc::Machine::VBLANK,
  		[] (gbc::Machine& machine, gbc::interrupt_t&)
  		{
        const int W = machine.gpu.SCREEN_W;
        const int H = machine.gpu.SCREEN_H;
        for (int y = 0; y < H; y++)
      	for (int x = 0; x < W; x++)
      	{
          const auto& pixels = machine.gpu.pixels();
          const uint8_t idx = pixels.at(W * y + x) & 0x3;
          uint8_t palette[] = {29, 25, 21, 17};
          set_pixel(x+80, y+32, palette[idx]);
        }
        // blit to front framebuffer here
        VGA_gfx::blit_from(backbuffer.data());
        vblanked = true;
      });
    // create a new frame
    while (vblanked == false)
  	{
  		machine->cpu.simulate();
  		machine->io.simulate();
  		machine->gpu.simulate();
  	}
    vblanked = false;
  });

  // input
  hw::KBM::set_virtualkey_handler(
  [] (int key, bool pressed)
  {
    static uint8_t keys = 0;
    switch (key) {
    case hw::KBM::VK_ENTER:
    case hw::KBM::VK_SPACE:
        setflag(pressed, keys, gbc::BUTTON_START);
        break;
    case hw::KBM::VK_BACK:
        setflag(pressed, keys, gbc::BUTTON_SELECT);
        break;
    case hw::KBM::VK_Z:
        setflag(pressed, keys, gbc::BUTTON_B);
        break;
    case hw::KBM::VK_X:
        setflag(pressed, keys, gbc::BUTTON_A);
        break;
    case hw::KBM::VK_UP:
        setflag(pressed, keys, gbc::DPAD_UP);
        break;
    case hw::KBM::VK_DOWN:
        setflag(pressed, keys, gbc::DPAD_DOWN);
        break;
    case hw::KBM::VK_RIGHT:
        setflag(pressed, keys, gbc::DPAD_RIGHT);
        break;
    case hw::KBM::VK_LEFT:
        setflag(pressed, keys, gbc::DPAD_LEFT);
        break;
    }
    machine->set_inputs(keys);
  });
}

extern "C" int getchar() {}
