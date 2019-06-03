#include <cstdint>
#include <cstring>
#include <map>
#include <x86intrin.h>

static std::array<uint8_t, 320 * 200> backbuffer __attribute__((aligned(16))) = {};
inline void set_pixel(int x, int y, uint8_t cl)
{
    // assert(x >= 0 && x < 320 && y >= 0 && y < 200);
    if (x >= 0 && x < 320 && y >= 0 && y < 200) backbuffer[y * 320 + x] = cl;
}
void gbz80_limited_blit(const uint8_t* backbuffer)
{
    auto* addr = (__m128i*) VGA_gfx::address();
    auto* src = (__m128i*) backbuffer;
    const int X = 80;
    const int W = 160;
    const int Y = 32;
    const int H = 144;

    for (int y = Y; y < Y + H; y++)
        for (int x = X; x < X + W; x += 16)
        {
            const int i = (y * 320 + x) / 16;
            _mm_stream_si128(&addr[i], src[i]);
        }
}
inline void clear(const uint8_t cl = 0)
{
    for (auto& idx : backbuffer) idx = cl;
}

struct rgb18_t
{
    int channel[3];

    static rgb18_t from_rgb15(uint16_t color);
    void curvify();
    void apply_palette(const uint8_t idx);
    uint32_t to_rgba() const;
};

#include <cmath>
inline void rgb18_t::curvify()
{
    float R = this->channel[0] / 63.0f;
    float G = this->channel[1] / 63.0f;
    float B = this->channel[2] / 63.0f;
    const float magn = sqrtf(R * R + G * G + B * B);

    R = powf(R, 0.93f) * (1.0f + 0.15f * magn);
    G = powf(G, 0.77f) * (1.0f + 0.15f * magn);
    B = powf(B, 0.77f) * (1.0f + 0.15f * magn);

    this->channel[0] = std::min(1.0f, R) * 63.0f;
    this->channel[1] = std::min(1.0f, G) * 63.0f;
    this->channel[2] = std::min(1.0f, B) * 63.0f;
}
inline rgb18_t rgb18_t::from_rgb15(uint16_t color)
{
    const uint8_t r = (color >> 0) & 0x1f;
    const uint8_t g = (color >> 5) & 0x1f;
    const uint8_t b = (color >> 10) & 0x1f;
    return rgb18_t{.channel = {r << 1, g << 1, b << 1}};
}
inline void rgb18_t::apply_palette(const uint8_t idx)
{
    VGA_gfx::set_palette(idx, channel[0], channel[1], channel[2]);
}
inline uint32_t rgb18_t::to_rgba() const
{
    return (channel[0] << 2) | (channel[1] << 10) | (channel[2] << 18);
}

static std::map<uint32_t, int> clr_trans;
inline int auto_indexed_rgba(const uint32_t color)
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
inline int auto_indexed_rgb18(rgb18_t& col)
{
    const uint32_t rgb = col.to_rgba();
    auto it = clr_trans.find(rgb);
    if (it != clr_trans.end())
    {
        // this color has a VGA index
        return it->second;
    }
    // create new index
    const int index = clr_trans.size();
    clr_trans[rgb] = index;
    col.apply_palette(index);
    return index;
}
inline void restart_indexing() { clr_trans.clear(); }
