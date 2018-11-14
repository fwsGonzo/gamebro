#include "stuff.hpp"
#include <libgbc/machine.hpp>
#include <bmp/bmp.h>
#include <signal.h>

static void save_screenshot(gbc::Machine& machine)
{
	// get pixel data
	const auto& pixels = machine.gpu.pixels();
	// render to BMP
	const char* filename = "screenshot.bmp";
	std::array<char, BMP_SIZE(160, 144)> array;
	bmp_init(array.data(), 160, 144);
	for (int y = 0; y < gbc::GPU::SCREEN_H; y++)
	for (int x = 0; x < gbc::GPU::SCREEN_W; x++)
	{
		const uint32_t px = pixels.at(y * gbc::GPU::SCREEN_W + x);
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
	//machine->cpu.default_pausepoint(0x203, 10, true);
	//machine->verbose_instructions = true;
	//machine->break_on_interrupts = true;
	//machine->stop_when_undefined = true;
	signal(SIGINT, int_handler);

	// wire up gameboy vblank
	machine->set_handler(gbc::Machine::VBLANK,
		[] (gbc::Machine& machine, gbc::interrupt_t&)
		{
			static int counter = 0;
			if (counter++ % 120 == 0) save_screenshot(machine);
		});

	while (machine->cpu.is_running())
	{
		machine->cpu.simulate();
		machine->io.simulate();
		machine->gpu.simulate();
	}
	save_screenshot(*machine);

  return 0;
}
