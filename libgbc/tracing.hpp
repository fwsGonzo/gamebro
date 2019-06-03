#pragma once
#include "util/delegate.hpp"
#include <cstdint>

namespace gbc
{
class CPU;

struct breakpoint_t
{
    delegate<void(CPU&, uint8_t)> callback;
};

} // namespace gbc
