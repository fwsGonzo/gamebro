#include <cstdint>
#include <cstring>

static std::array<uint8_t, 320*200> backbuffer __attribute__((aligned(16))) = {};
inline void set_pixel(int x, int y, uint8_t cl)
{
  //assert(x >= 0 && x < 320 && y >= 0 && y < 200);
  if (x >= 0 && x < 320 && y >= 0 && y < 200)
      backbuffer[y * 320 + x] = cl;
}
inline void clear()
{
  backbuffer = {};
}
