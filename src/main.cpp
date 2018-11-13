#include "stuff.hpp"
#include <libgbc/machine.hpp>
#include <bmp/bmp.h>

static void save_screenshot(gbc::Machine& machine)
{
	// get pixel data
	static std::vector<uint32_t> pixels;
	machine.gpu.render_to(pixels);
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

int main(int argc, char** args)
{
	const char* romfile = "tests/bits_ram_en.gb";
	if (argc >= 2) romfile = args[1];

  const auto romdata = load_file(romfile);
  printf("Loaded %zu bytes ROM\n", romdata.size());

	auto* m = new gbc::Machine(romdata);
	//m->verbose_instructions = true;
	//m->cpu.default_pausepoint(0x299a, 0, false);
	//m->cpu.default_pausepoint(0x46e9, 10, true);
	//m->break_on_interrupts = true;
	//m->stop_when_undefined = true;
	bool brk = false;
	//m->cpu.breakpoint(0x7d19, [&brk] (gbc::CPU&, uint8_t) {brk = true;});

	// wire up gameboy vblank
	m->set_handler(gbc::Machine::VBLANK,
		[] (gbc::Machine& machine, gbc::interrupt_t&)
		{
			static int counter = 0;
			if (counter++ % 60 == 0) save_screenshot(machine);
			// sleep on each vblank
			static uint64_t t0 = 0;
			const uint64_t t1 = machine.cpu.gettime();
			usleep((t1 - t0) / 4);
			t0 = t1;
		});

	while (m->cpu.is_running())
	{
		m->cpu.simulate();
		m->io.simulate();

		/*if (m->cpu.registers().pc == 0x3cd)
		{
			m->break_now();
			m->verbose_instructions = true;
			//m->cpu.break_on_steps(1);
		}*/

		if (brk) {
			static int counter = 0;
			if ((counter++ % 10) == 0) m->break_now();
		}
	}

  return 0;
}
