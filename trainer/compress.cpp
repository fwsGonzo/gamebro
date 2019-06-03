#include <lzo/lzo1c.h>
using buffer_t = std::vector<uint8_t>;
static size_t compress(const buffer_t& src, uint8_t* buffer, size_t buflen, int level = 1)
{
    std::array<uint8_t, LZO1C_MEM_COMPRESS> workmem;
    lzo_uint out_size = buflen;
    int res = lzo1c_compress(src.data(), src.size(), &buffer[2], &out_size, workmem.data(), level);
    assert(res == LZO_E_OK);
    *(uint16_t*) &buffer[0] = out_size;
    return 2 + out_size;
}

static void record_machine_state(FILE* outf, gbc::Machine* machine)
{
    // auto start = std::chrono::high_resolution_clock::now();
    state.clear();
    // serialize machine state
    machine->serialize_state(state);
    // compress and write to file
    std::array<uint8_t, 32768> cdata;
    size_t clen = compress(state, cdata.data(), cdata.size());
    fwrite(cdata.data(), clen, 1, outf);
    total_bytes += clen;
    // measure time taken
    // auto finish = std::chrono::high_resolution_clock::now();
    // std::chrono::duration<double> delta = finish - start;

    static size_t last_bytes = 0;
    if (total_bytes > last_bytes + 1024 * 1024)
    {
        last_bytes = total_bytes;
        printf("Compressed state is %zu bytes vs %zu raw (total %zu)\n", clen, state.size(),
               total_bytes);
        double t = machine->gpu.frame_count() * 0.0167;
        printf("Frames: %zu  Time: %.3f\n", machine->gpu.frame_count(), t);
    }
    // printf("Serialization and compression took %.5f millis\n", delta.count() * 1000.0);
}
