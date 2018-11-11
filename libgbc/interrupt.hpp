#pragma once
#include <cstdint>
#include <util/delegate.hpp>

namespace gbc
{
  struct interrupt_t;
  using interrupt_handler = delegate<void(interrupt_t&)>;

  struct interrupt_t {
    const uint8_t  mask;
    const uint16_t fixed_address;
    const char* name = "";
    uint64_t last_time = 0;
    int mode = 0;
    interrupt_handler callback = nullptr;

    interrupt_t(uint8_t msk, uint16_t addr, const char* n)
        : mask(msk), fixed_address(addr), name(n) {}
  };

}
