#pragma once
#include "memory.hpp"
#include <vector>

namespace gbc
{
  class DisplayData
  {
  public:
    DisplayData(Memory::range_t range) {
      m_data.resize(Memory::range_size(range));
      m_range = range;
    }

    uint8_t read(uint16_t address);
    void    write(uint16_t address, uint8_t value);

  private:
    std::vector<uint8_t> m_data;
    Memory::range_t m_range;
  };
}
