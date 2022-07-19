#pragma once
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

static inline std::vector<uint8_t> load_file(const std::string& filename)
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

template <typename C>
static inline void save_file(const std::string& filename, const C& data)
{
    FILE* f = fopen(filename.c_str(), "wb");
    if (f == NULL) throw std::runtime_error("Could not open file: " + filename);

    if (1 != fwrite(data.data(), data.size(), 1, f))
    {
        fclose(f);
        throw std::runtime_error("Error when writing file: " + filename);
    }
    fclose(f);
}

inline uint64_t micros_now()
{
    using namespace std::chrono;
    return duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
}
