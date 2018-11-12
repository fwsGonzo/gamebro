#include "stuff.hpp"
#include <libgbc/machine.hpp>
#include <bmp/bmp.h>

static void save_screenshot(gbc::Machine& machine)
{
	// get pixel data
	std::vector<uint32_t> pixels;
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
	//m->stop_when_undefined = true;
	//m->cpu.default_pausepoint(0x485c, true);
	//m->break_on_interrupts = true;
	bool brk = false;
	//m->cpu.breakpoint(0x7d19, [&brk] (gbc::CPU&, uint8_t) {brk = true;});

	// wire up gameboy vblank
	m->set_handler(gbc::Machine::VBLANK,
		[] (gbc::Machine& machine, gbc::interrupt_t&)
		{
			static int counter = 0;
			if (counter++ % 30 == 0) save_screenshot(machine);
		});

	while (m->cpu.is_running())
	{
		const uint64_t t0 = m->cpu.gettime();
		m->cpu.simulate();
		uint64_t t1 = m->cpu.gettime() - t0;
		//usleep(t1 * 50);
		m->io.simulate();
		//if (m->cpu.gettime() > 9000) assert(0);
		if (brk) {
			static int counter = 0;
			if ((counter++ % 10) == 0) m->break_now();
		}
	}

  return 0;
}
