#include "cpu.hpp"

#include "machine.hpp"
#include <cassert>
#include <cstring>
#include "instructions.cpp"

namespace gbc
{
  instruction_t& CPU::resolve_instruction(const uint8_t opcode)
  {
    if (opcode == 0) return instr_NOP;
    if (opcode == 0x08) return instr_LD_N_SP;

    if ((opcode & 0xc0) == 0x40) {
      if (opcode == 0x76) return instr_HALT;
      return instr_LD_D_D;
    }
    if ((opcode & 0xcf) == 0x1)  return instr_LD_R_N;
    if ((opcode & 0xe7) == 0x2)  return instr_LD_R_A_R;
    if ((opcode & 0xc7) == 0x3)  return instr_INC_DEC_R;
    if ((opcode & 0xc6) == 0x4)  return instr_INC_DEC_D;
    if (opcode == 0x10) return instr_STOP;
    if (opcode == 0x18) return instr_JR_N;
    if ((opcode & 0xe7) == 0x20) return instr_JR_N;
    if ((opcode & 0xc7) == 0x6)  return instr_LD_D_N;
    if ((opcode & 0xe7) == 0x22) return instr_LDID_HL_A;
    if ((opcode & 0xf7) == 0x37) return instr_SCF_CCF;
    if ((opcode & 0xc6) == 0xc6) return instr_ALU_A_N_D;
    if ((opcode & 0xc0) == 0x80) return instr_ALU_A_N_D;
    if ((opcode & 0xcb) == 0xc1) return instr_PUSH_POP;
    if ((opcode & 0xe7) == 0xc0) return instr_RET; // cond ret
    if (opcode == 0xc9) return instr_RET;          // plain ret
    if ((opcode & 0xc7) == 0xc7) return instr_RST;
    if ((opcode & 0xff) == 0xc3) return instr_JP; // direct
    if ((opcode & 0xe7) == 0xc2) return instr_JP; // conditional
    if ((opcode & 0xff) == 0xc4) return instr_CALL; // direct
    if ((opcode & 0xcd) == 0xcd) return instr_CALL; // conditional
    if ((opcode & 0xef) == 0xea) return instr_LD_N_A_N;
    if ((opcode & 0xef) == 0xe0) return instr_LD_xxx_A; // FF00+N
    if ((opcode & 0xef) == 0xe2) return instr_LD_xxx_A; // C
    if ((opcode & 0xef) == 0xea) return instr_LD_xxx_A; // N
    if ((opcode & 0xf7) == 0xf3) return instr_DI_EI;
    // instruction set extension opcodes
    if (opcode == 0xcb) return instr_CB_EXT;

    return instr_MISSING;
  }

  CPU::CPU(Memory& mem) noexcept
    : m_memory(mem)
  {
    this->reset();
  }

  void CPU::reset() noexcept
  {
    // gameboy Z80 initial register values
    registers().af = 0x01b0;
    registers().bc = 0x0013;
    registers().de = 0x00d8;
    registers().hl = 0x014d;
    registers().sp = 0xfffe;
    registers().pc = 0x0100;
    this->m_cycles_total = 0;
  }

  uint8_t CPU::readop8(const int dx)
  {
    return memory().read8(registers().pc + dx);
  }
  uint16_t CPU::readop16(const int dx)
  {
    return memory().read16(registers().pc + dx);
  }

  void CPU::simulate()
  {
    // 1. read instruction from memory
    const uint8_t opcode = this->readop8(0);
    // 2. execute instruction
    unsigned time = this->execute(opcode);
    // 3. pass the time (in T-states)
    this->incr_cycles(time);
  }

  unsigned CPU::execute(const uint8_t opcode)
  {
    if (this->m_singlestep) {
      // pause for each instruction
      this->print_and_pause(*this, opcode);
    }
    else {
      // look for breakpoints
      auto it = m_breakpoints.find(registers().pc);
      if (it != m_breakpoints.end()) {
        /*unsigned ret =*/ it->second(*this, opcode);
      }
    }
    // print the instruction (when enabled)
    char prn[128];
    auto& instr = resolve_instruction(opcode);
    instr.printer(prn, sizeof(prn), *this, opcode);
    printf("[pc 0x%04x] opcode 0x%02x: %s",
            registers().pc,  opcode, prn);
    // increment program counter
    registers().pc += 1;
    // run instruction handler
    unsigned ret = instr.handler(*this, opcode);
    // print out the resulting flags reg
    if (m_last_flags != registers().flags)
    {
      m_last_flags = registers().flags;
      char fbuf[5];
      printf(" -> F: [%s]\n",
              cstr_flags(fbuf, registers().flags));
    }
    else printf("\n");

    // return cycles used
    return ret;
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

  void CPU::print_and_pause(CPU& cpu, const uint8_t opcode)
  {
    char buffer[512];
    cpu.resolve_instruction(opcode) .printer(buffer, sizeof(buffer), cpu, opcode);
    printf("Breakpoint at [pc 0x%04x] opcode 0x%02x: %s\n",
           cpu.registers().pc, opcode, buffer);
    printf("%s", cpu.registers().to_string().c_str());
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
