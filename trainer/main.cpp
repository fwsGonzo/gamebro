#include <signal.h>
#include <chrono>
#include "../src/stuff.hpp"

#include <libgbc/machine.hpp>
static gbc::Machine* machine = nullptr;
static void int_handler(int) {
	machine->break_now();
}

#include <lzo/lzo1c.h>
using buffer_t = std::vector<uint8_t>;
static buffer_t compress(const buffer_t& src, size_t guess, int level = 1)
{
	std::array<uint8_t, LZO1C_MEM_COMPRESS> workmem;
	buffer_t result;
	result.resize(guess);
	lzo_uint out_size = result.size();
	int res = lzo1c_compress(src.data(), src.size(),
													 result.data(), &out_size,
													 workmem.data(), level);
	assert(res == LZO_E_OK);
	result.resize(out_size);
	return result;
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

	machine->io.on_joypad_read(
		[] (gbc::Machine& machine, int mode) {
			if (mode == 0) {
				//printf("Machine is about to read buttons\n");
				machine.set_inputs((rand() % 15) << 4);
			}
			else {
				//printf("Machine is about to read dpad\n");
				machine.set_inputs((rand() % 15) << 0);
			}
		});

	std::vector<uint8_t> state;
	while (machine->is_running())
	{
		machine->simulate();

		auto start = std::chrono::high_resolution_clock::now();
		state.clear();
		machine->serialize_state(state);
		auto compr = compress(state, 8192);
		auto finish = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> delta = finish - start;

		printf("Compressed Machine state is %zu bytes vs %zu raw\n", compr.size(), state.size());
		printf("Serialization and compression took %.5f millis\n", delta.count() * 1000.0);
	}
  return 0;
}
