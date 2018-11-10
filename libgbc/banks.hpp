#pragma once
#include <array>
#include <cstdint>
#include <cassert>
#include <vector>

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

  template <size_t SIZE>
  class ROMBank {
  public:
    ROMBank(const std::vector<uint8_t>& romvec, size_t off)
        : rom(&romvec), m_offset(off) {}

    uint8_t read(uint16_t addr) {
      assert(addr < SIZE);
      return rom->at(m_offset + addr);
    }

    void set_offset(size_t off) {
      assert((off % SIZE) == 0);
      assert((off + SIZE) <= rom->size());
      this->m_offset = off;
    }

  private:
    const std::vector<uint8_t>* rom = nullptr;
    size_t m_offset = 0;
  };
}
