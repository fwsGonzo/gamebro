#pragma once
#include "common.hpp"
#include <cstdint>
#include <vector>

namespace gbc
{
  class GPU
  {
  public:
    static const int SCREEN_W = 160;
    static const int SCREEN_H = 144;

    GPU(Machine&) noexcept;
    void reset() noexcept;
    // the vector is resized to exactly fit the screen
    void render_to(std::vector<uint32_t>& dest);

    Memory&  memory() noexcept { return m_memory; }

  private:
    uint32_t tile32(int x, int y);
    Memory& m_memory;
  };
}
