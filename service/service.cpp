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
  VGA_gfx::clear(0xff);
  VGA_gfx::apply_default_palette();
  clear(0xff); // use an unused black color

  fs::memdisk().init_fs(
  [] (fs::error_t err, fs::File_system&) {
    assert(!err);
  });
  auto& filesys = fs::memdisk().fs();
  //auto rombuffer = filesys.read_file("/tloz_la_dx.gbc");
  //auto rombuffer = filesys.read_file("/pokemon_yellow.gbc");
  auto rombuffer = filesys.read_file("/pokemon_crystal.gbc");
  //auto rombuffer = filesys.read_file("/tloz_seasons.gbc");
  //auto rombuffer = filesys.read_file("/tetris.gb");
  assert(rombuffer.is_valid());
  auto romdata = std::move(*rombuffer.get());

  static gbc::Machine* machine = nullptr;
  machine = new gbc::Machine(romdata);
  //machine->gpu.set_pixelmode(gbc::PM_RGBA);
  machine->gpu.set_pixelmode(gbc::PM_PALETTE);

  // trap on V-blank
  static bool vblanked = false;
  machine->set_handler(gbc::Machine::VBLANK,
    [] (gbc::Machine& machine, gbc::interrupt_t&)
    {
      const int W = machine.gpu.SCREEN_W;
      const int H = machine.gpu.SCREEN_H;
      //restart_indexing();

      for (int y = 0; y < H; y++)
      for (int x = 0; x < W; x++)
      {
        const auto& pixels = machine.gpu.pixels();
        // Palette mode
        const uint32_t idx = pixels.at(W * y + x);
        if (machine.is_cgb()) {
          assert((idx & 0xff) == idx);
          set_pixel(x+80, y+32, idx);
        }
        else {
          const uint8_t palette[] = {29, 25, 21, 17};
          set_pixel(x+80, y+32, palette[idx & 0x3]);
        }
        // RGBA mode
        //const uint32_t color = pixels.at(W * y + x);
        //int index = auto_indexed_color(color);
        //set_pixel(x+80, y+32, index);
      }
      // blit to front framebuffer here
      VGA_gfx::blit_from(backbuffer.data());
      vblanked = true;
    });

  machine->gpu.on_palchange(
    [] (const uint8_t idx, const uint16_t color)
    {
      const uint8_t r = (color >>  0) & 0x1f;
      const uint8_t g = (color >>  5) & 0x1f;
      const uint8_t b = (color >> 10) & 0x1f;
      VGA_gfx::set_palette(idx, r << 1, g << 1, b << 1);
    });

  // framebuffer
  using namespace std::chrono;
  Timers::periodic(16ms,
  [] (int) {
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
  hw::KBM::init();
  hw::KBM::set_virtualkey_handler(
  [] (int key, bool pressed)
  {
    static uint8_t keys = 0;
    switch (key) {
    case hw::KBM::VK_ENTER:
    case hw::KBM::VK_SPACE:
        gbc::setflag(pressed, keys, gbc::BUTTON_START);
        break;
    case hw::KBM::VK_BACK:
        gbc::setflag(pressed, keys, gbc::BUTTON_SELECT);
        break;
    case hw::KBM::VK_Z:
        gbc::setflag(pressed, keys, gbc::BUTTON_B);
        break;
    case hw::KBM::VK_X:
        gbc::setflag(pressed, keys, gbc::BUTTON_A);
        break;
    case hw::KBM::VK_UP:
        gbc::setflag(pressed, keys, gbc::DPAD_UP);
        break;
    case hw::KBM::VK_DOWN:
        gbc::setflag(pressed, keys, gbc::DPAD_DOWN);
        break;
    case hw::KBM::VK_RIGHT:
        gbc::setflag(pressed, keys, gbc::DPAD_RIGHT);
        break;
    case hw::KBM::VK_LEFT:
        gbc::setflag(pressed, keys, gbc::DPAD_LEFT);
        break;
    }
    machine->set_inputs(keys);
  });
}

// getchar() stub
extern "C" int getchar() { return 0; }
