///////////////////////////////////
// Gamebro Emulator as a Service //
///////////////////////////////////
#include <service>
#include <timers>
#include <memdisk>
#include <hw/vga_gfx.hpp>
#include <hw/ps2.hpp>

#include <machine.hpp>
static gbc::Machine* machine = nullptr;
static bool vblanked = false;
static void timer_function(int);
void timer_function(int)
{
  // create a new frame
  while (vblanked == false)
  {
    machine->simulate();
  }
  vblanked = false;
  Timers::oneshot(std::chrono::milliseconds(16), timer_function);
}

#include "backbuffer.cpp"
void Service::start()
{
  VGA_gfx::set_mode(VGA_gfx::MODE_320_200_256);
  VGA_gfx::clear(0xff);
  clear(0xff); // use an unused black color

  fs::memdisk().init_fs(
  [] (fs::error_t err, fs::File_system&) {
    assert(!err);
  });
  auto& filesys = fs::memdisk().fs();
  auto rombuffer = filesys.read_file("/ucity.gbc");
  //auto rombuffer = filesys.read_file("/tloz_seasons.gbc");
  //auto rombuffer = filesys.read_file("/tloz_la_dx.gbc");
  //auto rombuffer = filesys.read_file("/tloz_la12.gb");
  //auto rombuffer = filesys.read_file("/smbland2.gb");
  //auto rombuffer = filesys.read_file("/pokemon_yellow.gbc");
  //auto rombuffer = filesys.read_file("/pokemon_gold.gbc");
  //auto rombuffer = filesys.read_file("/pokemon_crystal.gbc");
  //auto rombuffer = filesys.read_file("/tetris.gb");
  assert(rombuffer.is_valid());
  auto romdata = std::move(*rombuffer.get());

  // settings
  machine = new gbc::Machine(romdata);

  // trap on V-blank
  machine->set_handler(gbc::Machine::VBLANK,
    [] (gbc::Machine& machine, gbc::interrupt_t&)
    {
      const int W = machine.gpu.SCREEN_W;
      const int H = machine.gpu.SCREEN_H;

      for (int y = 0; y < H; y++)
      for (int x = 0; x < W; x++)
      {
        const auto& pixels = machine.gpu.pixels();
        // Palette mode
        const uint32_t idx = pixels.at(W * y + x);
        set_pixel(x+80, y+32, idx);
      }
      // blit to front framebuffer here
      gbz80_limited_blit(backbuffer.data());
      vblanked = true;
    });

  if (!machine->is_cgb())
  {
    // constant 4-color palette
    auto colors = machine->gpu.dmg_colors(gbc::LIGHTER_GREEN);
    for (int i = 0; i < 4; i++)
      VGA_gfx::set_pal24(i, colors[i]);
  }
  else
  {
    // build palette in real-time
    machine->gpu.on_palchange(
      [] (const uint8_t idx, const uint16_t color)
      {
        rgb18_t rgb = rgb18_t::from_rgb15(color);
        if (machine->is_cgb()) rgb.curvify();
        rgb.apply_palette(idx);
      });
  }

  // framebuffer
  Timers::periodic(std::chrono::milliseconds(16),
   [] (int) {
     // create a new frame
     while (vblanked == false)
     {
       machine->simulate();
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
    case hw::KBM::VK_ESCAPE:
    case hw::KBM::VK_BACK:
        gbc::setflag(pressed, keys, gbc::BUTTON_SELECT);
        break;
    case hw::KBM::VK_1:
    case hw::KBM::VK_Z:
        gbc::setflag(pressed, keys, gbc::BUTTON_B);
        break;
    case hw::KBM::VK_2:
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
