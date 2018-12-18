#include <signal.h>
#include "../src/stuff.hpp"

#include <libgbc/machine.hpp>
static gbc::Machine* machine = nullptr;
static void int_handler(int) {
	machine->break_now();
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

	while (machine->is_running())
	{
		machine->simulate();
	}
  return 0;
}
