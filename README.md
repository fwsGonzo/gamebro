## GBC emulator library and trainer written in modern C++

### Load a ROM
```C++
    std::vector<uint8_t> romdata = load_file(...);
    auto* machine = new gbc::Machine(romdata);
```

### Pixel output
Intended for embedded where you have direct access to framebuffers. Your computer needs a way to get a high-precision timestamp and sleep for micros at a time. Delegates will be called async from the virtual machine, and you must call the system calls from there. You can tell the virtual machine about key presses through the API.


The virtual machine does not copy the ROM data unnecessarily, and uses memory conservatively. The instruction decoder is written to reduce instruction count. When building with clang you should enable LTO.

```C++
    // trap on palette writes, resulting in a 15-bit RGB color
    machine->gpu.on_palchange(
        [] (const uint8_t idx, const uint16_t color)
        {
            // do some magic with the 5-5-5 bit channels
        });
    // trap on V-blank interrupts
    machine->set_handler(gbc::Machine::VBLANK,
        [] (gbc::Machine& machine, gbc::interrupt_t&)
        {
            // Retrieve a vector of indexed colors
            const auto& pixels = machine.gpu.pixels();

            for (int y = 0; y < gbc::GPU::SCREEN_H; y++)
            for (int x = 0; x < gbc::GPU::SCREEN_W; x++)
            {
                const uint16_t idx = pixels.at(gbc::GPU::SCREEN_W * y + x);
                // in mode13h just write index directly to backbuffer
                set_pixel(x+80, y+32, idx);
            }
            // blit and flip front framebuffer here
            /* gbz80_limited_blit(...); */
        });

```
You can assume that the index gbc::GPU::WHITE_IDX is always a white color.

### GB color palettes

The emulator has some predefined color palettes for GB.
```C++
    enum dmg_variant_t {
        LIGHTER_GREEN = 0,
        DARKER_GREEN,
        GRAYSCALE
    };
    machine.gpu.set_dmg_variant(...);
    // now we can expand color indices on V-blanks
    uint32_t color = machine.gpu.expand_dmg_color(idx);
```
If you don't trap on palette writes in CGB mode, you can still expand CGB color indices like this:
```C++
    // Expand CGB color index to 32-bit RGB (with no alpha):
    uint32_t color = machine.gpu.expand_cgb_color(idx);
```

### 16-bit pixel buffer

The pixel buffer is 16-bits so that it can fit 15-bit colors if anyone wants to re-add the support. It is somewhat costly to do it this way without using macros. The emulator used to support a wide variety of color modes, but it's too costly to maintain and the vast majority of GBC games don't change palettes mid-frame, even though they can. You can replace some code in the GPU, specifically the return values of colorize_tile() and colorize_sprite(), to just write the color directly to the pixel buffer. The method to computing a CGB color is simply:
```C++
  uint16_t rgb15 = this->getpal(index*2) | (this->getpal(index*2+1) << 8);
```
You can also use bit 15 for something extra.

### Debugging
Run the command-line variant in your favorite OS, and press Ctrl+C to break into a debugger. Only caveat is that the break is always at the next instruction.

```C++
#include <libgbc/machine.hpp>
#include <signal.h>

static gbc::Machine* machine = nullptr;
static void int_handler(int) {
	machine->break_now();
}

int main(int argc, char** args)
{
	machine = new gbc::Machine(romvector); // load this yourself
	machine->break_now();                  // start in breakpoint
	signal(SIGINT, int_handler);           // break on Ctrl+C

	while (machine->is_running())
	{
		machine->simulate();
	}
    return 0;
}

```

### Trainer
We can use reinforcement learning with full machine-inspection to train a neural network to play games well. Use cheat searching in other GUI-based emulators to get memory addresses that can be used as rewards.

### Post-mortem tidbits after writing a GBC emulator

[Click here to read POSTERITY.md](POSTERITY.md)
