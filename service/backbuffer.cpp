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

#include <cmath>
inline void color_curvify(int rgb[3])
{
  float R = rgb[0] / 63.0f;
  float G = rgb[1] / 63.0f;
  float B = rgb[2] / 63.0f;
  const float magn = sqrtf(R*R + G*G + B*B);

  R = powf(R, 0.93f) * (1.0f + 0.15f * magn);
  G = powf(G, 0.77f) * (1.0f + 0.15f * magn);
  B = powf(B, 0.77f) * (1.0f + 0.15f * magn);

  rgb[0] = std::min(1.0f, R) * 63.0f;
  rgb[1] = std::min(1.0f, G) * 63.0f;
  rgb[2] = std::min(1.0f, B) * 63.0f;
}
