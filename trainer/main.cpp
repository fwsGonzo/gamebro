#include <libgbc/machine.hpp>
#include <bmp/bmp.h>
#include <signal.h>
#include "../src/stuff.hpp"

static gbc::Machine* machine = nullptr;
static void int_handler(int) {
	if (machine->cpu.is_breaking()) machine->cpu.stop();
	else machine->break_now();
}

int main(int argc, char** args)
{
	const char* romfile = "../smbland2.gb";
	if (argc >= 2) romfile = args[1];

  const auto romdata = load_file(romfile);
  printf("Loaded %zu bytes ROM\n", romdata.size());

	machine = new gbc::Machine(romdata);
	machine->break_now();
	signal(SIGINT, int_handler);

	uint64_t last_frame = 0;
	while (machine->is_running())
	{
		machine->simulate();
	}
  return 0;
}
