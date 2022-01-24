#include <unistd.h>
static std::vector<uint8_t> file_loader(const std::string& filename)
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
        fclose(f);
        throw std::runtime_error("Error when reading from file: " + filename);
    }
    fclose(f);
    return result;
}
static void file_writer(const std::string& filename, const std::vector<uint8_t>& data)
{
    FILE* f = fopen(filename.c_str(), "wb");
    if (f == NULL) throw std::runtime_error("Could not open file: " + filename);

	size_t size = data.size();
    if (size != fwrite(data.data(), 1, size, f))
    {
        fclose(f);
        throw std::runtime_error("Error when reading from file: " + filename);
    }
    fclose(f);
}

static std::vector<uint8_t> create_serialized_state();
static void restore_state_from(const std::vector<uint8_t> state);
