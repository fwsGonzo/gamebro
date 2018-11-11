#include "cpu.hpp"

#include "machine.hpp"
#include <cassert>
#include "instructions.cpp"

namespace gbc
{
  CPU::CPU(Memory& mem) noexcept
    : m_memory(mem), m_machine(mem.machine())
  {
    this->reset();
  }

  void CPU::reset() noexcept
  {
    // gameboy Z80 initial register values
    registers().af = 0x11b0;
    registers().bc = 0x0113;
    registers().de = 0x00d8;
    registers().hl = 0x014d;
    registers().sp = 0xfffe;
    registers().pc = 0x0100;
    this->m_cycles_total = 0;
  }

  void CPU::simulate()
  {
    // 1. read instruction from memory
    this->m_cur_opcode = this->readop8(0);
    // 2. execute instruction
    unsigned time = this->execute(this->m_cur_opcode);
    // 3. pass the time (in T-states)
    this->incr_cycles(time);
    // 4. handle interrupts
    this->handle_interrupts();
  }

  unsigned CPU::execute(const uint8_t opcode)
  {
    if (UNLIKELY(this->m_singlestep || this->m_break)) {
      // pause for each instruction
      this->print_and_pause(*this, opcode);
      this->m_break = false;
    }
    else if (UNLIKELY(!m_breakpoints.empty())) {
      // look for breakpoints
      auto it = m_breakpoints.find(registers().pc);
      if (it != m_breakpoints.end()) {
        /*unsigned ret =*/ it->second(*this, opcode);
      }
    }
    // decode into executable instruction
    auto& instr = decode(opcode);

    // print the instruction (when enabled)
    if (UNLIKELY(machine().verbose_instructions))
    {
      char prn[128];
      instr.printer(prn, sizeof(prn), *this, opcode);
      printf("%9lu: [pc 0x%04x] opcode 0x%02x: %s\n",
              gettime(), registers().pc,  opcode, prn);
    }

    // increment program counter
    registers().pc += 1;
    // run instruction handler
    unsigned ret = instr.handler(*this, opcode);

    if (UNLIKELY(machine().verbose_instructions))
    {
      // print out the resulting flags reg
      if (m_last_flags != registers().flags)
      {
        m_last_flags = registers().flags;
        char fbuf[5];
        printf("* Flags changed: [%s]\n",
                cstr_flags(fbuf, registers().flags));
      }
    }
    // return cycles used
    return ret;
  }

  // it takes 2 instruction-cycles to toggle interrupts
  void CPU::enable_interrupts() noexcept {
    m_intr_enable_pending = 2;
  }
  void CPU::disable_interrupts() noexcept {
    m_intr_disable_pending = 2;
  }

  void CPU::handle_interrupts()
  {
    // enable/disable interrupts over cycles
    if (m_intr_enable_pending > 0) {
      m_intr_enable_pending--;
      if (!m_intr_enable_pending) m_intr_master_enable = true;
    }
    if (m_intr_disable_pending > 0) {
      m_intr_disable_pending--;
      if (!m_intr_disable_pending) m_intr_master_enable = false;
    }
    const uint8_t imask = machine().io.interrupt_mask();
    // check if interrupts are enabled and pending
    if (this->ime() && imask != 0x0)
    {
      // 5. execute pending interrupts
      auto& io = machine().io;
      if (imask &  0x1) io.interrupt(io.vblank);
      if (imask &  0x2) io.interrupt(io.lcd_stat);
      if (imask &  0x4) io.interrupt(io.timer);
      if (imask &  0x8) io.interrupt(io.serial);
      if (imask & 0x10) io.interrupt(io.joypad);
    }
  }

  uint8_t CPU::readop8(const int dx)
  {
    return memory().read8(registers().pc + dx);
  }
  uint16_t CPU::readop16(const int dx)
  {
    return memory().read16(registers().pc + dx);
  }

  instruction_t& CPU::decode(const uint8_t opcode)
  {
    if (opcode == 0) return instr_NOP;
    if (opcode == 0x08) return instr_LD_N_SP;

    if ((opcode & 0xc0) == 0x40) {
      if (opcode == 0x76) return instr_HALT;
      return instr_LD_D_D;
    }
    if ((opcode & 0xcf) == 0x1)  return instr_LD_R_N;
    else if ((opcode & 0xe7) == 0x2)  return instr_LD_R_A_R;
    else if ((opcode & 0xcf) == 0x9)  return instr_ADD_HL_R;
    else if ((opcode & 0xc7) == 0x3)  return instr_INC_DEC_R;
    else if ((opcode & 0xc6) == 0x4)  return instr_INC_DEC_D;
    else if ((opcode & 0xe7) == 0x7)  return instr_RLC_RRC;
    else if (opcode == 0x10) return instr_STOP;
    else if (opcode == 0x18) return instr_JR_N;
    else if ((opcode & 0xe7) == 0x20) return instr_JR_N;
    else if ((opcode & 0xc7) == 0x6)  return instr_LD_D_N;
    else if ((opcode & 0xe7) == 0x22) return instr_LDID_HL_A;
    else if (opcode == 0x2f)          return instr_CPL;
    else if ((opcode & 0xf7) == 0x37) return instr_SCF_CCF;
    else if ((opcode & 0xc7) == 0xc6) return instr_ALU_A_N_D;
    else if ((opcode & 0xc0) == 0x80) return instr_ALU_A_N_D;
    else if ((opcode & 0xcb) == 0xc1) return instr_PUSH_POP;
    else if ((opcode & 0xe7) == 0xc0) return instr_RET; // cond ret
    else if ((opcode & 0xef) == 0xc9) return instr_RET; // ret / reti
    else if ((opcode & 0xc7) == 0xc7) return instr_RST;
    else if ((opcode & 0xff) == 0xc3) return instr_JP; // direct
    else if ((opcode & 0xe7) == 0xc2) return instr_JP; // conditional
    else if ((opcode & 0xff) == 0xc4) return instr_CALL; // direct
    else if ((opcode & 0xcd) == 0xcd) return instr_CALL; // conditional
    else if ((opcode & 0xef) == 0xea) return instr_LD_N_A_N;
    else if ((opcode & 0xef) == 0xe0) return instr_LD_xxx_A; // FF00+N
    else if ((opcode & 0xef) == 0xe2) return instr_LD_xxx_A; // C
    else if ((opcode & 0xef) == 0xea) return instr_LD_xxx_A; // N
    else if (opcode == 0xf8) return instr_LD_HL_SP; // N
    else if ((opcode & 0xef) == 0xe9) return instr_LD_JP_HL;
    else if ((opcode & 0xf7) == 0xf3) return instr_DI_EI;
    // instruction set extension opcodes
    else if (opcode == 0xcb) return instr_CB_EXT;
    else return instr_MISSING;
  }

  uint8_t CPU::read_hl() {
    return memory().read8(registers().hl);
  }
  void CPU::write_hl(const uint8_t value) {
    memory().write8(registers().hl, value);
  }

  void CPU::incr_cycles(int count)
  {
    assert(count >= 0);
    this->m_cycles_total += count;
  }

  void CPU::stop()
  {
    this->m_running = false;
  }

  void CPU::wait()
  {
    this->m_waiting = true;
  }

  unsigned CPU::push_and_jump(uint16_t address)
  {
    registers().sp -= 2;
    memory().write16(registers().sp, registers().pc);
    registers().pc = address;
    return 8;
  }

  void CPU::print_and_pause(CPU& cpu, const uint8_t opcode)
  {
    char buffer[512];
    cpu.decode(opcode).printer(buffer, sizeof(buffer), cpu, opcode);
    printf("\n");
    printf(">>> Breakpoint at [pc 0x%04x] opcode 0x%02x: %s\n",
           cpu.registers().pc, opcode, buffer);
    // CPU registers
    printf("%s\n", cpu.registers().to_string().c_str());
    // I/O interrupt registers
    auto& io = cpu.machine().io;
    printf("\tIF = 0x%02x  IE = 0x%02x  IME 0x%x\n",
           io.read_io(IO::REG_IF), io.read_io(IO::REG_IE), cpu.ime());
    try {
      auto& mem = cpu.memory();
      printf("\t(HL) = 0x%04x  (SP) = 0x%04x  (0xA000) = 0x%04x\n",
            mem.read16(cpu.registers().hl), mem.read16(cpu.registers().sp),
            mem.read16(0xA000));
    } catch (...) {}
    printf("Press any key to continue...\n");
    getchar(); // press any key
  }

}
