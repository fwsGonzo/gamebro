#include "stuff.hpp"
#include <libgbc/machine.hpp>
#include <bmp/bmp.h>

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
		[] (gbc::interrupt_t& intr) {
			// render to BMP
			const char* filename = "screenshot.bmp";
			std::vector<char> array(bmp_size(160, 144));
			bmp_init(array.data(), 160, 144);
			save_file(filename, array);
			printf("*** Stored screenshot in %s\n", filename);
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
