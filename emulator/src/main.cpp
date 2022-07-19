#include "stuff.hpp"
#include <bmp/bmp.h>
#include <libgbc/machine.hpp>
#include <signal.h>

static void save_screenshot(const char* filename, const std::vector<uint16_t>& pixels)
{
    int size_x = 0, size_y = 0;
    if (pixels.size() == 160 * 144)
    {
        size_x = 160;
        size_y = 144;
    }
    else if (pixels.size() == 256 * 256)
    {
        size_x = 256;
        size_y = 256;
    }
    else if (pixels.size() == 128 * 192)
    {
        size_x = 128;
        size_y = 192;
    }
    else
        assert(0 && "Unknown size");
    // render to BMP
    std::array<char, BMP_SIZE(256, 256)> array;
    bmp_init(array.data(), size_x, size_y);
    for (int y = 0; y < size_y; y++)
    for (int x = 0; x < size_x; x++)
    {
        const uint16_t color = pixels.at(y * size_x + x);
        const uint32_t r = ((color >> 0) & 0x1f) << 3;
        const uint32_t g = ((color >> 5) & 0x1f) << 3;
        const uint32_t b = ((color >> 10) & 0x1f) << 3;
        const uint32_t rgba = (r << 16) | (g << 8) | (b << 0);
        bmp_set(array.data(), x, y, rgba);
    }
    // save it!
    save_file(filename, array);
    printf("*** Stored screenshot in %s\n", filename);
}

static gbc::Machine* machine = nullptr;
static void int_handler(int)
{
    if (machine->cpu.is_breaking())
        machine->cpu.stop();
    else
        machine->break_now();
}

int main(int argc, char** args)
{
    const char* romfile = "tests/instr_timing.gb";
    if (argc >= 2) romfile = args[1];

    const auto romdata = load_file(romfile);
    printf("Loaded %zu bytes ROM\n", romdata.size());

    machine = new gbc::Machine(romdata);
    machine->gpu.scanline_rendering(false);
    machine->break_now();
    /*
    //machine->cpu.default_pausepoint(0x453);
    machine->memory.breakpoint(gbc::Memory::READ,
                    [] (gbc::Memory& mem, uint16_t addr, uint8_t) {
                            if (addr == 0xDF40) {
                                    printf("Something is reading from %04X\n", addr);
                                    mem.machine().break_now();
                            }
                    });
    machine->memory.breakpoint(gbc::Memory::WRITE,
                    [] (gbc::Memory& mem, uint16_t addr, uint8_t value) {
                            if (addr == 0xDF40) {
                                    printf("Something is writing %02X to %04X\n", value, addr);
                                    mem.machine().break_now();
                            }
                    });
    */
    // machine->cpu.default_pausepoint(0x3b89);
    // machine->verbose_banking = true;
    // machine->verbose_instructions = true;
    // machine->break_on_interrupts = true;
    // machine->stop_when_undefined = true;
    signal(SIGINT, int_handler);

    machine->set_handler(gbc::Machine::DEBUG, [](gbc::Machine& machine, gbc::interrupt_t&) {
        // render a full frame before we can make a screenshot
        machine.gpu.scanline_rendering(true);
        machine.simulate_one_frame();
        machine.gpu.scanline_rendering(false);
        static const char* filename = "screenshot.bmp";
        save_screenshot(filename, machine.gpu.pixels());
        // dump background & tiles for this frame
        const char* bgfile = "background.bmp";
        save_screenshot(bgfile, machine.gpu.dump_background());
        const char* tilefile = "tiles0.bmp";
        save_screenshot(tilefile, machine.gpu.dump_tiles(0));
        if (machine.is_cgb())
        {
            const char* tilefile = "tiles1.bmp";
            save_screenshot(tilefile, machine.gpu.dump_tiles(1));
        }
    });

    while (machine->is_running()) { machine->simulate(); }
    return 0;
}
