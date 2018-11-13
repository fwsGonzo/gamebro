// only include this file once!
#include "cpu.hpp"
#include "common.hpp"
#include "printers.hpp"
#define DEF_INSTR(x) static instruction_t instr_##x {handler_##x, printer_##x}
#define INSTRUCTION(x) static unsigned handler_##x
#define PRINTER(x) static int printer_##x

namespace gbc
{
  INSTRUCTION(MISSING) (CPU&, const uint8_t opcode)
  {
    fprintf(stderr, "Missing instruction: %#x\n", opcode);
    assert(0 && "Unimplemented instruction reached");
  }
  PRINTER(MISSING) (char* buffer, size_t len, CPU&, const uint8_t) {
    return snprintf(buffer, len, "MISSING");
  }

  INSTRUCTION(NOP) (CPU&, const uint8_t)
  {
    return 4; // NOP takes 4 T-states
  }
  PRINTER(NOP) (char* buffer, size_t len, CPU&, const uint8_t) {
    return snprintf(buffer, len, "NOP");
  }

  INSTRUCTION(LD_N_SP) (CPU& cpu, const uint8_t)
  {
    cpu.registers().sp = cpu.readop16(0);
    cpu.registers().pc += 2;
    return 4;
  }
  PRINTER(LD_N_SP) (char* buffer, size_t len, CPU& cpu, const uint8_t) {
    return snprintf(buffer, len, "LD (0x%04x), SP", cpu.readop16(1));
  }

  INSTRUCTION(LD_R_N) (CPU& cpu, const uint8_t opcode)
  {
    cpu.registers().getreg_sp(opcode) = cpu.readop16(0);
    cpu.registers().pc += 2;
    return 4;
  }
  PRINTER(LD_R_N) (char* buffer, size_t len, CPU& cpu, uint8_t opcode) {
    return snprintf(buffer, len, "LD %s, 0x%04x",
                    cstr_reg(opcode, true), cpu.readop16(1));
  }

  INSTRUCTION(ADD_HL_R) (CPU& cpu, const uint8_t opcode)
  {
    auto& reg   = cpu.registers().getreg_sp(opcode);
    auto& hl    = cpu.registers().hl;
    auto& flags = cpu.registers().flags;
    flags &= ~MASK_NEGATIVE;
    if (((hl &  0x0fff) + (reg &  0x0fff)) &  0x1000) flags |= MASK_HALFCARRY;
    if (((hl & 0x0ffff) + (reg & 0x0ffff)) & 0x10000) flags |= MASK_CARRY;
    hl += reg;
    return 8;
  }
  PRINTER(ADD_HL_R) (char* buffer, size_t len, CPU&, uint8_t opcode) {
    return snprintf(buffer, len, "ADD HL, %s", cstr_reg(opcode, true));
  }

  INSTRUCTION(LD_R_A_R) (CPU& cpu, const uint8_t opcode)
  {
    if (opcode & 4) {
      cpu.memory().write16(cpu.registers().getreg_sp(opcode), cpu.registers().accum);
    } else {
      cpu.registers().accum = cpu.memory().read16(cpu.registers().getreg_sp(opcode));
    }
    return 8;
  }
  PRINTER(LD_R_A_R) (char* buffer, size_t len, CPU&, uint8_t opcode) {
    if (opcode & 4) {
      return snprintf(buffer, len, "LD (%s), A", cstr_reg(opcode, true));
    }
    return snprintf(buffer, len, "LD A, (%s)", cstr_reg(opcode, true));
  }

  INSTRUCTION(INC_DEC_R) (CPU& cpu, const uint8_t opcode)
  {
    auto& reg = cpu.registers().getreg_sp(opcode);
    auto& flags = cpu.registers().flags;
    flags &= ~MASK_NEGATIVE;
    if ((opcode & 0x8) == 0) {
      reg++;
    } else {
      reg--;
      flags |= MASK_NEGATIVE; // DEC
    }
    if (reg == 0) flags |= MASK_ZERO; // set zero
    return 8;
  }
  PRINTER(INC_DEC_R) (char* buffer, size_t len, CPU&, uint8_t opcode) {
    if ((opcode & 0x8) == 0) {
      return snprintf(buffer, len, "INC %s", cstr_reg(opcode, true));
    }
    return snprintf(buffer, len, "DEC %s", cstr_reg(opcode, true));
  }

  INSTRUCTION(INC_DEC_D) (CPU& cpu, const uint8_t opcode)
  {
    const uint8_t dst = opcode >> 3;
    uint8_t  value;
    unsigned cycles;
    if (dst != 0x6)
    {
      if ((opcode & 0x1) == 0) {
        cpu.registers().getdest(dst)++;
      }
      else {
        cpu.registers().getdest(dst)--;
      }
      value = cpu.registers().getdest(dst);
      cycles = 8;
    }
    else {
      if ((opcode & 0x1) == 0) {
        cpu.write_hl(cpu.read_hl() + 1);
      } else {
        cpu.write_hl(cpu.read_hl() - 1);
      }
      value = cpu.read_hl();
      cycles = 16;
    }
    auto& flags = cpu.registers().flags;
    flags &= MASK_CARRY; // keep carry
    if (opcode & 0x1) flags |= MASK_NEGATIVE; // DEC
    if (value == 0) flags |= MASK_ZERO; // set zero
    return cycles;
  }
  PRINTER(INC_DEC_D) (char* buffer, size_t len, CPU&, uint8_t opcode) {
    const char* mnemonic = (opcode & 0x1) ? "DEC" : "INC";
    return snprintf(buffer, len, "%s %s", mnemonic, cstr_dest(opcode >> 3));
  }

  INSTRUCTION(LD_D_N) (CPU& cpu, const uint8_t opcode)
  {
    if (((opcode >> 3) & 0x7) != 0x6)
      cpu.registers().getdest(opcode >> 3) = cpu.readop8(0);
    else
      cpu.write_hl(cpu.readop8(0));
    cpu.registers().pc += 1;
    return 4;
  }
  PRINTER(LD_D_N) (char* buffer, size_t len, CPU& cpu, uint8_t opcode) {
    if (((opcode >> 3) & 0x7) != 0x6)
      return snprintf(buffer, len, "LD %s, 0x%02x",
                      cstr_dest(opcode >> 3), cpu.readop8(1));
    else
      return snprintf(buffer, len, "LD (HL=0x%04x), 0x%02x",
                      cpu.registers().hl, cpu.readop8(1));
  }

  INSTRUCTION(RLC_RRC) (CPU& cpu, const uint8_t opcode)
  {
    auto& accum = cpu.registers().accum;
    auto& flags = cpu.registers().flags;
    flags &= ~MASK_NEGATIVE;
    flags &= ~MASK_HALFCARRY;
    switch (opcode) {
      case 0x07: {
        // RLCA, rotate A left
        const uint8_t bit7 = accum & 0x80;
        accum = (accum << 1) | (bit7 >> 7);
        flags &= ~0x80;
        flags |= bit7 >> 3; // old bit7 to CF
      } break;
      case 0x0F: {
        // RRCA, rotate A right
        const uint8_t bit0 = accum & 0x1;
        accum = (accum >> 1) | (bit0 << 7);
        flags &= ~0x1;
        flags |= bit0 << 4; // old bit0 to CF
      } break;
      case 0x17: {
        // RLA, rotate A left, old CF to bit 0
        const uint8_t bit7 = accum & 0x80;
        accum = (accum << 1) | ((flags & MASK_CARRY) >> 4);
        flags &= ~0x80;
        flags |= bit7 >> 3; // old bit7 to CF
      } break;
      case 0x1F: {
        // RRA, rotate A right, old CF to bit 7
        const uint8_t bit0 = accum & 0x1;
        accum = (accum >> 1) | ((flags & MASK_CARRY) << 3);
        flags &= ~0x1;
        flags |= bit0 << 4; // old bit0 to CF
      } break;
      default:
        assert(0 && "Unknown opcode in RLC/RRC handler");
    }
    if (accum == 0) flags |= MASK_ZERO;
    return 4;
  }
  PRINTER(RLC_RRC) (char* buffer, size_t len, CPU& cpu, uint8_t opcode) {
    const char* mnemonic[4] = {"RLC", "RRC", "RL", "RR"};
    return snprintf(buffer, len, "%s A (A = 0x%02x)",
                    mnemonic[opcode >> 3], cpu.registers().accum);
  }

  INSTRUCTION(LD_D_D) (CPU& cpu, const uint8_t opcode)
  {
    const bool HL = (opcode & 0x7) == 0x6;
    uint8_t reg;
    if (!HL) reg = cpu.registers().getdest(opcode);
    else     reg = cpu.read_hl();
    const unsigned cycles = (HL) ? 8 : 4;

    if (((opcode >> 3) & 0x7) != 0x6) {
        cpu.registers().getdest(opcode >> 3) = reg;
        return cycles;
    }
    cpu.write_hl(reg);
    return cycles + 4;
  }
  PRINTER(LD_D_D) (char* buffer, size_t len, CPU&, uint8_t opcode) {
    return snprintf(buffer, len, "LD %s, %s",
                    cstr_dest(opcode >> 3), cstr_dest(opcode >> 0));
  }

  INSTRUCTION(LD_N_A_N) (CPU& cpu, const uint8_t opcode)
  {
    const uint16_t addr = cpu.readop16(0);
    cpu.registers().pc += 2;
    if ((opcode & 0x10) == 0) {
      // load into (N) from A
      cpu.memory().write8(addr, cpu.registers().accum);
    }
    else {
      // load into A from (N)
      cpu.registers().accum = cpu.memory().read8(addr);
    }
    return 4;
  }
  PRINTER(LD_N_A_N) (char* buffer, size_t len, CPU& cpu, uint8_t opcode) {
    if ((opcode & 0x10) == 0)
      return snprintf(buffer, len, "LD (0x%04x), A", cpu.readop16(1));
    else
      return snprintf(buffer, len, "LD A, (0x%04x)", cpu.readop16(1));
  }

  INSTRUCTION(LDID_HL_A) (CPU& cpu, const uint8_t opcode)
  {
    if ((opcode & 0x8) == 0) {
      // load from A into (HL)
      cpu.write_hl(cpu.registers().accum);
    }
    else {
      // load from (HL) into A
      cpu.registers().accum = cpu.read_hl();
    }
    if ((opcode & 0x10) == 0) {
      cpu.registers().hl++;
    } else {
      cpu.registers().hl--;
    }
    return 8;
  }
  PRINTER(LDID_HL_A) (char* buffer, size_t len, CPU& cpu, uint8_t opcode)
  {
    const char* mnemonic = (opcode & 0x10) ? "LDD" : "LDI";
    if ((opcode & 0x8) == 0)
      return snprintf(buffer, len, "%s (HL=0x%04x), A", mnemonic, cpu.registers().hl);
    else
      return snprintf(buffer, len, "%s A, (HL=0x%04x)", mnemonic, cpu.registers().hl);
  }

  INSTRUCTION(CPL) (CPU& cpu, const uint8_t)
  {
    cpu.registers().accum = ~cpu.registers().accum;
    cpu.registers().flags |= MASK_NEGATIVE;
    cpu.registers().flags |= MASK_HALFCARRY;
    return 4;
  }
  PRINTER(CPL) (char* buffer, size_t len, CPU&, uint8_t) {
    return snprintf(buffer, len, "CPL");
  }

  INSTRUCTION(SCF_CCF) (CPU& cpu, const uint8_t opcode)
  {
    if ((opcode & 0x8) == 0) { // Set CF
      cpu.registers().flags |= MASK_CARRY;
    } else { // Complement CF
      cpu.registers().flags ^= MASK_CARRY;
    }
    cpu.registers().flags &= ~MASK_NEGATIVE;
    cpu.registers().flags &= ~MASK_HALFCARRY;
    return 4;
  }
  PRINTER(SCF_CCF) (char* buffer, size_t len, CPU&, uint8_t opcode) {
    return snprintf(buffer, len, (opcode & 0x8) ? "CCF (CY=0)" : "SCF (CY=1)");
  }

  // ALU A, D / A, N
  INSTRUCTION(ALU_A_N_D) (CPU& cpu, const uint8_t opcode)
  {
    const uint8_t alu_op = opcode >> 3;
    if ((opcode & 0x40) == 0)
    {
      // <alu> A, D
      if ((opcode & 0x7) != 0x6)
        cpu.registers().alu(alu_op, cpu.registers().getdest(opcode));
      else
        cpu.registers().alu(alu_op, cpu.read_hl());
      return 4;
    }
    // <alu> A, N
    cpu.registers().alu(alu_op, cpu.readop8());
    cpu.registers().pc++;
    return 8;
  }
  PRINTER(ALU_A_N_D) (char* buffer, size_t len, CPU& cpu, uint8_t opcode)
  {
    const uint8_t alu_op = opcode >> 3;
    if ((opcode & 0x40) == 0)
    {
      return snprintf(buffer, len, "%s A, %s",
                      cstr_alu(alu_op), cstr_dest(opcode));
    }
    return snprintf(buffer, len, "%s A, 0x%02x",
                    cstr_alu(alu_op), cpu.readop8(1));
  }

  INSTRUCTION(JP) (CPU& cpu, const uint8_t opcode)
  {
    if ((opcode & 1) || (cpu.registers().compare_flags(opcode))) {
      cpu.registers().pc = cpu.readop16(0);
      if (UNLIKELY(cpu.machine().verbose_instructions))
      {
        printf("* Jumped to 0x%04x\n", cpu.registers().pc);
      }
      return 8;
    }
    cpu.registers().pc += 2;
    return 8;
  }
  PRINTER(JP) (char* buffer, size_t len, CPU& cpu, uint8_t opcode) {
    if (opcode & 1) {
      return snprintf(buffer, len, "JP 0x%04x", cpu.readop16(1));
    }
    char temp[128];
    fill_flag_buffer(temp, sizeof(temp), opcode, cpu.registers().flags);
    return snprintf(buffer, len, "JPF 0x%04x (%s)",
                    cpu.readop16(1), temp);
  }

  INSTRUCTION(PUSH_POP) (CPU& cpu, const uint8_t opcode)
  {
    if (opcode & 4) {
      // PUSH R
      cpu.registers().sp -= 2;
      cpu.memory().write16(cpu.registers().sp, cpu.registers().getreg_af(opcode));
      return 16;
    }
    // POP R
    cpu.registers().getreg_af(opcode) = cpu.memory().read16(cpu.registers().sp);
    cpu.registers().sp += 2;
    return 12;
  }
  PRINTER(PUSH_POP) (char* buffer, size_t len, CPU& cpu, uint8_t opcode) {
    if (opcode & 4) {
      return snprintf(buffer, len, "PUSH %s (0x%04x)",
                      cstr_reg(opcode, false),
                      cpu.registers().getreg_af(opcode));
    }
    return snprintf(buffer, len, "POP %s (0x%04x)",
                    cstr_reg(opcode, false),
                    cpu.memory().read16(cpu.registers().sp));
  }

  INSTRUCTION(RET) (CPU& cpu, const uint8_t opcode)
  {
    if ((opcode & 0xef) == 0xc9 || cpu.registers().compare_flags(opcode)) {
      cpu.registers().pc = cpu.memory().read16(cpu.registers().sp);
      cpu.registers().sp += 2;
      if (UNLIKELY(cpu.machine().verbose_instructions))
      {
        printf("* Returned to 0x%04x\n", cpu.registers().pc);
      }
      // RETI
      if (UNLIKELY((opcode & 0x11) == 0x11)) {
        cpu.enable_interrupts();
      }
    }
    return 8;
  }
  PRINTER(RET) (char* buffer, size_t len, CPU& cpu, uint8_t opcode) {
    if ((opcode & 0xef) == 0xc9) {
      return snprintf(buffer, len, (opcode & 0x10) ? "RETI" : "RET");
    }
    char temp[128];
    fill_flag_buffer(temp, sizeof(temp), opcode, cpu.registers().flags);
    return snprintf(buffer, len, "RET %s", temp);
  }

  INSTRUCTION(RST) (CPU& cpu, const uint8_t opcode)
  {
    const uint16_t dst = opcode & 0x38;
    if (UNLIKELY(cpu.registers().pc == dst+1)) {
      printf(">>> RST loop detected at vector 0x%04x\n", dst);
      cpu.break_now();
      return 8;
    }
    // jump to vector area
    unsigned t = cpu.push_and_jump(dst);
    if (UNLIKELY(cpu.machine().verbose_instructions))
    {
      printf("* Restart vector 0x%04x\n", cpu.registers().pc);
    }
    return 4 + t;
  }
  PRINTER(RST) (char* buffer, size_t len, CPU&, uint8_t opcode) {
    return snprintf(buffer, len, "RST 0x%02x", opcode & 0x38);
  }

  INSTRUCTION(STOP) (CPU& cpu, const uint8_t)
  {
    cpu.stop();
    return 4;
  }
  PRINTER(STOP) (char* buffer, size_t len, CPU&, uint8_t) {
    return snprintf(buffer, len, "STOP");
  }

  INSTRUCTION(JR_N) (CPU& cpu, const uint8_t opcode)
  {
    if ((opcode & 0x20) == 0 || (cpu.registers().compare_flags(opcode))) {
      cpu.registers().pc += 1 + (int8_t) cpu.readop8(0);
      if (UNLIKELY(cpu.machine().verbose_instructions))
      {
        printf("* Jumped relative to 0x%04x\n", cpu.registers().pc);
      }
    } else {
      cpu.registers().pc += 1; // skip over imm8
    }
    return 8;
  }
  PRINTER(JR_N) (char* buffer, size_t len, CPU& cpu, uint8_t opcode) {
    const uint8_t disp = cpu.readop8(1);
    const uint16_t dest = cpu.registers().pc + 2 + disp;
    if (opcode & 0x20) {
      char temp[128];
      fill_flag_buffer(temp, sizeof(temp), opcode, cpu.registers().flags);
      return snprintf(buffer, len, "JR %+hhd (%s) => 0x%04x",
                      disp, temp, dest);
    }
    return snprintf(buffer, len, "JR %+hhd => 0x%04x", disp, dest);
  }

  INSTRUCTION(HALT) (CPU& cpu, const uint8_t)
  {
    cpu.wait();
    return 4;
  }
  PRINTER(HALT) (char* buffer, size_t len, CPU&, uint8_t) {
    return snprintf(buffer, len, "HALT");
  }

  INSTRUCTION(CALL) (CPU& cpu, const uint8_t opcode)
  {
    if ((opcode & 1) || cpu.registers().compare_flags(opcode)) {
      // push address of **next** instr
      cpu.registers().sp -= 2;
      cpu.memory().write16(cpu.registers().sp, 2 + cpu.registers().pc);
      // jump to immediate address
      cpu.registers().pc = cpu.readop16(0);
      if (UNLIKELY(cpu.machine().verbose_instructions))
      {
        printf("* Called 0x%04x\n", cpu.registers().pc);
      }
      return 20;
    }
    return 12;
  }
  PRINTER(CALL) (char* buffer, size_t len, CPU& cpu, uint8_t opcode) {
    if (opcode & 1) {
      return snprintf(buffer, len, "CALL 0x%04x", cpu.readop16(1));
    }
    char temp[128];
    fill_flag_buffer(temp, sizeof(temp), opcode, cpu.registers().flags);
    return snprintf(buffer, len, "CALLF 0x%04x (%s)",
                    cpu.readop16(1), temp);
  }

  INSTRUCTION(LD_xxx_A) (CPU& cpu, const uint8_t opcode)
  {
    const bool from_a = (opcode & 0x10) == 0;
    uint16_t addr;
    unsigned cycles;
    if ((opcode & 0xa) == 0x0) {
      addr = 0xFF00 + cpu.readop8(0);
      cpu.registers().pc += 1;
      cycles = 12;
    }
    else if ((opcode & 0xa) == 0x2) {
      addr = 0xFF00 + cpu.registers().c;
      cycles = 8;
    }
    else {
      addr = cpu.readop16(0);
      cpu.registers().pc += 2;
      cycles = 8;
    }
    if (from_a)
        cpu.memory().write8(addr, cpu.registers().accum);
    else
        cpu.registers().accum = cpu.memory().read8(addr);
    return cycles;
  }
  PRINTER(LD_xxx_A) (char* buffer, size_t len, CPU& cpu, uint8_t opcode)
  {
    const bool from_a = (opcode & 0x10) == 0;
    if ((opcode & 0xa) == 0x0) {
      if (from_a)
        return snprintf(buffer, len, "LD (FF00+0x%02x), A", cpu.readop8(1));
      else
        return snprintf(buffer, len, "LD A, (FF00+0x%02x)", cpu.readop8(1));
    }
    else if ((opcode & 0xa) == 0x2) {
      if (from_a)
        return snprintf(buffer, len, "LD (FF00+0x%02x), A", cpu.registers().c);
      else
        return snprintf(buffer, len, "LD A, (FF00+0x%02x)", cpu.registers().c);
    }
    if (from_a)
      return snprintf(buffer, len, "LD (0x%04x), A", cpu.readop16(1));
    else
      return snprintf(buffer, len, "LD A, (0x%04x)", cpu.readop16(1));
  }

  INSTRUCTION(LD_HL_SP) (CPU& cpu, const uint8_t)
  {
    // LD HL, SP+N
    cpu.registers().hl = cpu.registers().sp + cpu.readop8(0);
    cpu.registers().pc++;
    return 12;
  }
  PRINTER(LD_HL_SP) (char* buffer, size_t len, CPU& cpu, uint8_t) {
    return snprintf(buffer, len,
                    "LD HL, SP + 0x%02x", cpu.readop8(1));
  }

  INSTRUCTION(LD_JP_HL) (CPU& cpu, const uint8_t opcode)
  {
    if (opcode & 0x10) {
      // LD SP, HL
      cpu.registers().sp = cpu.registers().hl;
      return 8;
    }
    else {
      // JP HL
      cpu.registers().pc = cpu.registers().hl;
      if (UNLIKELY(cpu.machine().verbose_instructions))
      {
        printf("* Jumped to 0x%04x\n", cpu.registers().pc);
      }
      return 4;
    }
  }
  PRINTER(LD_JP_HL) (char* buffer, size_t len, CPU& cpu, uint8_t opcode) {
    const char* mnemonic = (opcode & 0x10) ? "LD SP, HL" : "JP HL";
    return snprintf(buffer, len, "%s = 0x%04x", mnemonic, cpu.registers().hl);
  }

  INSTRUCTION(DI_EI) (CPU& cpu, const uint8_t opcode)
  {
    if (opcode & 0x08) {
      cpu.enable_interrupts();
    }
    else {
      cpu.disable_interrupts();
    }
    return 4;
  }
  PRINTER(DI_EI) (char* buffer, size_t len, CPU&, uint8_t opcode) {
    const char* mnemonic = (opcode & 0x10) ? "DI" : "EI";
    return snprintf(buffer, len, "%s", mnemonic);
  }

  INSTRUCTION(CB_EXT) (CPU& cpu, const uint8_t)
  {
    const uint8_t opcode = cpu.readop8(0);
    cpu.registers().pc++;
    const bool HL = (opcode & 0x7) == 0x6;
    uint8_t reg;
    if (!HL) reg = cpu.registers().getdest(opcode);
    else     reg = cpu.read_hl();
    const unsigned cycles = (HL) ? 16 : 8;

    // BIT, RESET, SET
    if (opcode >> 6)
    {
      const uint8_t bit = (opcode >> 3) & 3;
      switch (opcode >> 6)
      {
        case 0x1: { // BIT
          const bool set = reg & (1 << bit);
          // set flags
          if (set == 0) cpu.registers().flags |= MASK_ZERO;
          cpu.registers().flags &= ~MASK_NEGATIVE;
          cpu.registers().flags |=  MASK_HALFCARRY;
          // BIT only takes 12 T-cycles for (HL)
          return (HL) ? 12 : 8;
          }
        case 0x2: // RESET
          reg &= ~(1 << bit);
          break;
        case 0x3: // SET
          reg |= 1 << bit;
          break;
      }
    }
    else if ((opcode & 0xF0) == 0x10)
    {
      auto& flags = cpu.registers().flags;
      flags &= ~MASK_NEGATIVE;
      flags &= ~MASK_HALFCARRY;
      flags &= ~MASK_CARRY;
      if (opcode & 0x8) {
        // RR D, rotate D right, old CF to bit 7
        const uint8_t bit0 = reg & 0x1;
        reg = (reg >> 1) | ((cpu.registers().flags & MASK_CARRY) << 3);
        flags |= bit0 << 4; // old bit0 to CF
      }
      else {
        // RL D, rotate D left, old CF to bit 0
        const uint8_t bit7 = reg & 0x80;
        reg = (reg << 1) | ((cpu.registers().flags & MASK_CARRY) >> 4);
        flags |= bit7 >> 3; // old bit7 to CF
      }
      if (reg == 0) cpu.registers().flags |= MASK_ZERO;
    }
    else if ((opcode & 0xF0) == 0x20)
    {
      auto& flags = cpu.registers().flags;
      flags &= ~MASK_NEGATIVE;
      flags &= ~MASK_HALFCARRY;
      flags &= ~MASK_CARRY;
      // TODO: fix flags
      if (opcode & 0x8) {
        // SRA
        const uint8_t bit0 = reg & 0x1;
        reg = (reg >> 1) & (reg & 0x80);
        flags |= bit0 << 4; // bit0 into CF
      }
      else {
        // SLA
        const uint8_t bit7 = reg & 0x80;
        reg <<= 1;
        flags |= bit7 >> 3; // bit7 into CF
      }
      if (reg == 0) cpu.registers().flags |= MASK_ZERO;
    }
    else if ((opcode & 0xf0) == 0x30)
    {
      if ((opcode & 0x8) == 0x0) {
        // SWAP D
        const uint8_t low = reg & 0xF;
        reg = (reg >> 4) | (low << 4);
        cpu.registers().flags &= MASK_ZERO;
        if (reg == 0) cpu.registers().flags |= MASK_ZERO;
      }
      else {
        // SRL D
        auto& flags = cpu.registers().flags;
        flags &= ~MASK_NEGATIVE;
        flags &= ~MASK_HALFCARRY;
        const uint8_t bit0 = reg & 0x1;
        reg >>= 1;
        flags &= ~MASK_CARRY;
        flags |= bit0 << 4; // bit0 into CF
        if (reg == 0) flags |= MASK_ZERO;
      }
    }
    else {
      fprintf(stderr, "Missing instruction: %#x\n", opcode);
      assert(0 && "Unimplemented extended instruction");
    }
    // all instructions on this opcode go into the same dest
    if (!HL) cpu.registers().getdest(opcode) = reg;
    else     cpu.write_hl(reg);
    return cycles;
  }
  PRINTER(CB_EXT) (char* buffer, size_t len, CPU& cpu, uint8_t)
  {
    const uint8_t opcode = cpu.readop8(1);
    if (opcode >> 6)
    {
      const char* mnemonic[] = {"IMPLEMENT ME", "BIT", "RES", "SET"};
      return snprintf(buffer, len, "%s %u, %s", mnemonic[opcode >> 6],
                      (opcode >> 3) & 0x7, cstr_dest(opcode));
    }
    else
    {
      const char* mnemonic[] = {"RLC", "RRC", "RL", "RR", "SLA", "SRA", "SWAP", "SRL"};
      return snprintf(buffer, len, "%s %s",
                      mnemonic[opcode >> 3], cstr_dest(opcode));
    }
  }

  DEF_INSTR(NOP);
  DEF_INSTR(LD_N_SP);
  DEF_INSTR(LD_R_N);
  DEF_INSTR(ADD_HL_R);
  DEF_INSTR(LD_R_A_R);
  DEF_INSTR(INC_DEC_R);
  DEF_INSTR(INC_DEC_D);
  DEF_INSTR(LD_D_N);
  DEF_INSTR(LD_D_D);
  DEF_INSTR(LD_N_A_N);
  DEF_INSTR(JR_N);
  DEF_INSTR(RLC_RRC);
  DEF_INSTR(LDID_HL_A);
  DEF_INSTR(CPL);
  DEF_INSTR(SCF_CCF);
  DEF_INSTR(HALT);
  DEF_INSTR(ALU_A_N_D);
  DEF_INSTR(PUSH_POP);
  DEF_INSTR(RST);
  DEF_INSTR(RET);
  DEF_INSTR(STOP);
  DEF_INSTR(JP);
  DEF_INSTR(CALL);
  DEF_INSTR(LD_xxx_A);
  DEF_INSTR(LD_HL_SP);
  DEF_INSTR(LD_JP_HL);
  DEF_INSTR(DI_EI);
  DEF_INSTR(CB_EXT);
  DEF_INSTR(MISSING);
}
