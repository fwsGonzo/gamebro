#pragma once
#include <array>
#include <cstdint>

namespace gbc
{
  template <int SIZE>
  struct Bank
  {
    std::array<uint8_t, SIZE>  m_bits = {};
  };

  template <int N, int SIZE>
  class Banks
  {
  public:
    using Bank = Bank<SIZE>;

    Bank&    get() { return m_banks.at(this->selected); }
    uint8_t& get(uint16_t idx) { return get().m_bits.at(idx); }

    void  select(int n) { this->selected = n; }

  private:
    std::array<Bank, N> m_banks;
    int  selected = 0;
    bool writable = true;
  };
}
