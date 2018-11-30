#include <cstdint>
#include <cstring>
#include <map>

static std::array<uint8_t, 320*200> backbuffer __attribute__((aligned(16))) = {};
inline void set_pixel(int x, int y, uint8_t cl)
{
  //assert(x >= 0 && x < 320 && y >= 0 && y < 200);
  if (x >= 0 && x < 320 && y >= 0 && y < 200)
      backbuffer[y * 320 + x] = cl;
}
inline void clear(const uint8_t cl = 0)
{
  for (auto& idx : backbuffer) idx = cl;
}
static std::map<uint32_t, int> clr_trans;
inline int auto_indexed_color(uint32_t color)
{
  auto it = clr_trans.find(color);
  if (it != clr_trans.end())
  {
    // this color has a VGA index
    return it->second;
  }
  // create new index
  const int index = clr_trans.size();
  clr_trans[color] = index;
  VGA_gfx::set_pal24(index, color);
  return index;
}
inline void restart_indexing()
{
  clr_trans.clear();
}
