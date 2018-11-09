#include <cstdio>
#include <stdexcept>
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

#include <libgbc/machine.hpp>
int main(int argc, char** args)
{
	const char* romfile = "tests/bits_ram_en.gb";
	if (argc >= 2) romfile = args[1];

  const auto romdata = load_file(romfile);
  printf("Loaded %zu bytes ROM\n", romdata.size());

	auto* m = new gbc::Machine(romdata);
	//m->cpu.default_pausepoint(0x485c, true);
	while (m->cpu.is_running())
	{
		m->cpu.simulate();
		//if (m->cpu.gettime() > 9000) assert(0);
	}

  return 0;
}
