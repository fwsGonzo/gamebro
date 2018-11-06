#include <cstdio>
#include <string>
#include <stdexcept>
static const char* romfile = "tloz_seasons.gbc";

static inline
std::string load_file(const std::string& filename)
{
	size_t size = 0;
	FILE* f = fopen(filename.c_str(), "rb");
	if (f == NULL) throw std::runtime_error("Could not open file: " + filename);

	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);

  std::string result;
  result.resize(size);
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

	auto* m = new gbc::Machine;
	while (true)
	{
		m->cpu.simulate();
	}

  return 0;
}
