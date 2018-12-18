#pragma once
#include <array>
#include <cstdint>
#include <util/delegate.hpp>
#include "common.hpp"

namespace gbc
{
  class APU
  {
  public:
    APU(Machine& mach);
    using audio_stream_t = delegate<void(uint16_t, uint16_t)>;

    void on_audio_out(audio_stream_t);
    void simulate();

    uint8_t read(uint16_t, uint8_t& reg);
    void    write(uint16_t, uint8_t, uint8_t& reg);

    Machine& machine() noexcept { return m_machine; }
  private:
    struct generator_t {

    };
    struct channel_t {
      bool generators_enabled[4] = {false};
      bool enabled = true;
    };

    Machine& m_machine;
    audio_stream_t m_audio_out;
  };
}
