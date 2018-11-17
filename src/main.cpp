#include "stuff.hpp"
#include <libgbc/machine.hpp>
#include <bmp/bmp.h>
#include <signal.h>

static void save_screenshot(const char* filename,
														const std::vector<uint32_t>& pixels)
{
	int size_x = 0, size_y = 0;
	if (pixels.size() == 160 * 144) {
		size_x = 160; size_y = 144;
	}
	else if (pixels.size() == 256 * 256) {
		size_x = 256; size_y = 256;
	}
	else if (pixels.size() == 128 * 192) {
		size_x = 128; size_y = 192;
	}
	else assert(0 && "Unknown size");
	// render to BMP
	std::array<char, BMP_SIZE(256, 256)> array;
	bmp_init(array.data(), size_x, size_y);
	for (int y = 0; y < size_y; y++)
	for (int x = 0; x < size_x; x++)
	{
		const uint32_t px = pixels.at(y * size_x + x);
		bmp_set(array.data(), x, y, px);
	}
	// save it!
	save_file(filename, array);
	printf("*** Stored screenshot in %s\n", filename);
}

static gbc::Machine* machine = nullptr;
static void int_handler(int) {
	if (machine->cpu.is_breaking()) machine->cpu.stop();
	else machine->break_now();
}

int main(int argc, char** args)
{
	const char* romfile = "tests/bits_ram_en.gb";
	if (argc >= 2) romfile = args[1];

  const auto romdata = load_file(romfile);
  printf("Loaded %zu bytes ROM\n", romdata.size());

	machine = new gbc::Machine(romdata);
	machine->break_now();
	//machine->cpu.default_pausepoint(0x204);
	//machine->verbose_instructions = true;
	//machine->break_on_interrupts = true;
	//machine->stop_when_undefined = true;
	signal(SIGINT, int_handler);

	// wire up gameboy vblank
	machine->set_handler(gbc::Machine::VBLANK,
		[] (gbc::Machine& machine, gbc::interrupt_t&)
		{
			const char* filename = "screenshot.bmp";
			static int counter = 0;
			if (counter++ % 120 == 0)
			save_screenshot(filename, machine.gpu.pixels());
		});
	machine->set_handler(gbc::Machine::DEBUG,
		[] (gbc::Machine& machine, gbc::interrupt_t&)
		{
			const char* bgfile = "background.bmp";
			save_screenshot(bgfile, machine.gpu.dump_background());
			const char* tilefile = "tiles.bmp";
			save_screenshot(tilefile, machine.gpu.dump_tiles());
		});

	while (machine->cpu.is_running())
	{
		machine->cpu.simulate();
		machine->io.simulate();
		machine->gpu.simulate();
	}
	save_screenshot("exitshot.bmp", machine->gpu.pixels());

  return 0;
}
