#pragma once
#include <cstdint>

namespace gbc
{
  class Machine;

  class IO
  {
  public:
    IO(Machine&);
    void    write_io(const uint16_t, uint8_t);
    uint8_t read_io(const uint16_t);

    void interrupt(uint8_t lane);

    void trigger_key(const uint8_t);

  private:
    Machine& m_machine;
    uint8_t  m_p1;
    
    uint8_t  m_timer = 0x0;
    uint8_t  m_interrupts = 0x0;
  };
}
