// only include this file once!
#include "cpu.hpp"
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

  INSTRUCTION(LD_R_A_R) (CPU& cpu, const uint8_t opcode)
  {
    if (opcode & 4) {
      cpu.memory().write16(cpu.registers().getreg_sp(opcode), cpu.registers().accum);
    } else {
      cpu.registers().accum = cpu.memory().read16(cpu.registers().getreg_sp(opcode));
    }
    return 8;
  }
  PRINTER(LD_R_A_R) (char* buffer, size_t len, CPU& cpu, uint8_t opcode) {
    if (opcode & 4) {
      return snprintf(buffer, len, "LD (%s), A", cstr_reg(opcode, true));
    }
    return snprintf(buffer, len, "LD A, (%s)", cstr_reg(opcode, true));
  }

  INSTRUCTION(INC_DEC_R) (CPU& cpu, const uint8_t opcode)
  {
    if ((opcode & 0x8) == 0) {
      cpu.registers().getreg_sp(opcode)++;
    } else {
      cpu.registers().getreg_sp(opcode)--;
    }
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
    if ((opcode & 0x1) == 0) {
      cpu.registers().getdest(opcode)++;
    } else {
      cpu.registers().getdest(opcode)--;
    }
    return 8;
  }
  PRINTER(INC_DEC_D) (char* buffer, size_t len, CPU&, uint8_t opcode) {
    if ((opcode & 0x1) == 0) {
      return snprintf(buffer, len, "INC %s", cstr_dest(opcode));
    }
    return snprintf(buffer, len, "DEC %s", cstr_dest(opcode));
  }

  INSTRUCTION(LD_D_N)
      (CPU& cpu, const uint8_t opcode)
  {
    if (((opcode >> 3) & 0x7) != 0x6)
      cpu.registers().getdest(opcode >> 3) = cpu.readop8(0);
    else
      cpu.memory().write8(cpu.registers().hl, cpu.readop8(0));
    cpu.registers().pc += 1;
    return 4;
  }
  PRINTER(LD_D_N) (char* buffer, size_t len, CPU& cpu, uint8_t opcode) {
    return snprintf(buffer, len, "LD %s, 0x%02x",
                    cstr_dest(opcode >> 3), cpu.readop8(1));
  }

  INSTRUCTION(LD_D_D) (CPU& cpu, const uint8_t opcode)
  {
    cpu.registers().getdest(opcode >> 3) = cpu.registers().getdest(opcode & 3);
    return 4;
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
      // load from (N) into A
      cpu.registers().accum = cpu.memory().read8(addr);
    }
    else {
      // load from A into (N)
      cpu.memory().write8(addr, cpu.registers().accum);
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
      // load from (HL) into A
      cpu.registers().accum = cpu.memory().read8(cpu.registers().hl);
    }
    else {
      // load from A into (HL)
      cpu.memory().write8(cpu.registers().hl, cpu.registers().accum);
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
      return snprintf(buffer, len, "%s (HL=0x%04x), A", mnemonic, cpu.readop16(1));
    else
      return snprintf(buffer, len, "%s A, (HL=0x%04x)", mnemonic, cpu.readop16(1));
  }

  // ALU A, D / A, N
  INSTRUCTION(ALU_A_N_D) (CPU& cpu, const uint8_t opcode)
  {
    const uint8_t alu_op = opcode >> 3;
    if ((opcode & 3) != 0x6) // 110
    {
      // <alu> A, D
      if ((opcode & 0x7) != 0x6)
        cpu.registers().alu(alu_op, cpu.registers().getdest(opcode));
      else
        cpu.registers().alu(alu_op, cpu.memory().read8(cpu.registers().hl));
      return 4;
    }
    // <alu> A, N
    cpu.registers().alu(alu_op, cpu.readop8());
    return 8;
  }
  PRINTER(ALU_A_N_D) (char* buffer, size_t len, CPU& cpu, uint8_t opcode)
  {
    const uint8_t alu_op = opcode >> 3;
    if ((opcode & 3) != 0x6) // 110
    {
      return snprintf(buffer, len, "%s A, %s",
                      cstr_alu(alu_op), cstr_dest(opcode));
    }
    return snprintf(buffer, len, "%s A, 0x%02x",
                    cstr_alu(alu_op), cpu.readop16(1));
  }

  INSTRUCTION(JP) (CPU& cpu, const uint8_t opcode)
  {
    if ((opcode & 1) || (cpu.registers().compare_flags(opcode))) {
      cpu.registers().pc = cpu.readop16(0);
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

  INSTRUCTION(CALL) (CPU& cpu, const uint8_t opcode)
  {
    if ((opcode & 1) || cpu.registers().compare_flags(opcode)) {
      // push address of next instr
      cpu.registers().sp -= 2;
      cpu.memory().write16(cpu.registers().sp, cpu.registers().pc);
      // jump to immediate address
      cpu.registers().pc = cpu.readop16(0);
      printf("* Call %#x => op %#x\n",
            cpu.registers().pc, cpu.memory().read8(cpu.registers().pc));
      return 16;
    }
    cpu.registers().pc += 2;
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
                      cpu.registers().getreg_af(opcode >> 4));
    }
    return snprintf(buffer, len, "POP %s (0x%04x)",
                    cstr_reg(opcode, false),
                    cpu.memory().read16(cpu.registers().sp));
  }

  INSTRUCTION(RET) (CPU& cpu, const uint8_t opcode)
  {
    if (opcode == 0xc9 || cpu.registers().compare_flags(opcode)) {
      cpu.registers().pc = cpu.memory().read16(cpu.registers().sp);
      cpu.registers().sp += 2;
      printf("* Returned to 0x%04x\n", cpu.registers().pc);
    }
    return 8;
  }
  PRINTER(RET) (char* buffer, size_t len, CPU& cpu, uint8_t opcode) {
    if (opcode == 0xc9) {
      return snprintf(buffer, len, "RET");
    }
    char temp[128];
    fill_flag_buffer(temp, sizeof(temp), opcode, cpu.registers().flags);
    return snprintf(buffer, len, "RET %s", temp);
  }

  INSTRUCTION(RST) (CPU& cpu, const uint8_t opcode)
  {
    // jump to vector area
    cpu.registers().sp -= 2;
    cpu.memory().write16(cpu.registers().sp, cpu.registers().pc);
    cpu.registers().pc = opcode & 0x38;
    printf("New PC: %#x\n", cpu.registers().pc);
    return 8;
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

  INSTRUCTION(JR_N) (CPU& cpu, const uint8_t)
  {
    cpu.registers().pc += 1 + (int8_t) cpu.readop8(0);
    return 8;
  }
  PRINTER(JR_N) (char* buffer, size_t len, CPU& cpu, uint8_t) {
    return snprintf(buffer, len, "JR 0x%04x (%+hhd)",
                    cpu.readop8(1), cpu.readop8(1));
  }

  INSTRUCTION(HALT) (CPU& cpu, const uint8_t)
  {
    cpu.wait();
    return 4;
  }
  PRINTER(HALT) (char* buffer, size_t len, CPU&, uint8_t) {
    return snprintf(buffer, len, "HALT");
  }

  DEF_INSTR(NOP);
  DEF_INSTR(LD_N_SP);
  DEF_INSTR(LD_R_N);
  DEF_INSTR(LD_R_A_R);
  DEF_INSTR(INC_DEC_R);
  DEF_INSTR(INC_DEC_D);
  DEF_INSTR(LD_D_N);
  DEF_INSTR(LD_D_D);
  DEF_INSTR(LD_N_A_N);
  DEF_INSTR(LDID_HL_A);
  DEF_INSTR(HALT);
  DEF_INSTR(ALU_A_N_D);
  DEF_INSTR(PUSH_POP);
  DEF_INSTR(RST);
  DEF_INSTR(RET);
  DEF_INSTR(STOP);
  DEF_INSTR(JR_N);
  DEF_INSTR(JP);
  DEF_INSTR(CALL);
  DEF_INSTR(MISSING);
}
