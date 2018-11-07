#include <cstdio>
#include <stdexcept>
#include <vector>
static const char* romfile = "tloz_la.gb";

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
int main()
{
  auto romdata = load_file(romfile);
  printf("Loaded %zu bytes ROM\n", romdata.size());

	auto* m = new gbc::Machine(romdata);
	while (m->cpu.is_running())
	{
		m->cpu.simulate();
	}

  return 0;
}
