#include <cstdio>
#include <chrono>
#include <stdexcept>
#include <string>
#include <vector>

static inline
std::vector<uint8_t> load_file(const std::string& filename)
{
	size_t size = 0;
	FILE* f = fopen(filename.c_str(), "rb");
	if (f == NULL) throw std::runtime_error("Could not open file: " + filename);

	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);

  std::vector<uint8_t> result(size);
	if (size != fread(result.data(), 1, size, f))
	{
    throw std::runtime_error("Error when reading from file: " + filename);
	}
	fclose(f);
	return result;
}

inline uint64_t micros_now() {
  using namespace std::chrono;
  return duration_cast<microseconds>(
      system_clock::now().time_since_epoch()
  ).count();
}

#include <libgbc/machine.hpp>
#include <unistd.h>
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
