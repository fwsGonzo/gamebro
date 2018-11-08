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
    return 4; // T-states
  }
  PRINTER(LD_N_SP) (char* buffer, size_t len, CPU& cpu, const uint8_t) {
    return snprintf(buffer, len, "LD (0x%04x), SP", cpu.readop16(1));
  }

  INSTRUCTION(LD_R_N) (CPU& cpu, const uint8_t opcode)
  {
    cpu.registers().getreg_sp(opcode) = cpu.readop16(0);
    cpu.registers().pc += 2;
    return 4; // T-states
  }
  PRINTER(LD_R_N) (char* buffer, size_t len, CPU& cpu, uint8_t opcode) {
    return snprintf(buffer, len, "LD %s, 0x%04x",
                    cstr_reg(opcode, true), cpu.readop16(1));
  }

  INSTRUCTION(LD_R_A_R) (CPU& cpu, const uint8_t opcode)
  {
    cpu.registers().getreg_sp(opcode) = cpu.readop16(0);
    cpu.registers().pc += 2;
    return 4; // T-states
  }
  PRINTER(LD_R_A_R) (char* buffer, size_t len, CPU& cpu, uint8_t opcode) {
    return snprintf(buffer, len, "LD %s, 0x%04x",
                    cstr_reg(opcode, true), cpu.readop16(1));
  }

  INSTRUCTION(LD_D_N)
      (CPU& cpu, const uint8_t opcode)
  {
    cpu.registers().getdest(opcode >> 3) = cpu.readop8(0);
    cpu.registers().pc += 1;
    return 4; // T-states
  }
  PRINTER(LD_D_N) (char* buffer, size_t len, CPU& cpu, uint8_t opcode) {
    return snprintf(buffer, len, "LD %s, 0x%02x",
                    cstr_dest(opcode >> 3), cpu.readop8(1));
  }

  INSTRUCTION(LD_D_D) (CPU& cpu, const uint8_t opcode)
  {
    cpu.registers().getdest(opcode >> 3) = cpu.registers().getdest(opcode & 3);
    cpu.registers().pc += 1;
    return 4; // T-states
  }
  PRINTER(LD_D_D) (char* buffer, size_t len, CPU& cpu, uint8_t opcode) {
    return snprintf(buffer, len, "LD %s, %s",
                    cstr_dest(opcode >> 3), cstr_dest(opcode >> 0));
  }

  INSTRUCTION(LD_N_A_N) (CPU& cpu, const uint8_t opcode)
  {
    const uint16_t addr = cpu.readop16(0);
    if ((opcode & 0x10) == 0) {
      // load from (N) into A
      cpu.registers().accum = cpu.memory().read8(addr);
    }
    else {
      // load from A into (N)
      cpu.memory().write8(addr, cpu.registers().accum);
    }
    return 4; // T-states
  }
  PRINTER(LD_N_A_N) (char* buffer, size_t len, CPU& cpu, uint8_t opcode) {
    if ((opcode & 0x10) == 0)
      return snprintf(buffer, len, "LD (0x%04x), A", cpu.readop16(1));
    else
      return snprintf(buffer, len, "LD A, (0x%04x)", cpu.readop16(1));
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
      cpu.memory().write16(cpu.registers().sp, cpu.registers().pc);
      // jump to immediate address
      cpu.registers().pc = cpu.readop16(0);
      printf("* Call %#x => op %#x\n",
            cpu.registers().pc, cpu.memory().read8(cpu.registers().pc));
      return 8;
    }
    cpu.registers().pc += 2;
    return 8; // T-states
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

  INSTRUCTION(RST) (CPU& cpu, const uint8_t opcode)
  {
    // jump to vector area
    cpu.memory().write16(cpu.registers().sp, cpu.registers().pc);
    cpu.registers().pc = opcode & 0x38;
    printf("New PC: %#x\n", cpu.registers().pc);
    return 8; // T-states
  }
  PRINTER(RST) (char* buffer, size_t len, CPU&, uint8_t opcode) {
    return snprintf(buffer, len, "RST 0x%02x", opcode & 0x38);
  }

  INSTRUCTION(STOP) (CPU& cpu, const uint8_t)
  {
    cpu.stop();
    return 4; // T-states
  }
  PRINTER(STOP) (char* buffer, size_t len, CPU&, uint8_t) {
    return snprintf(buffer, len, "STOP");
  }

  INSTRUCTION(HALT) (CPU& cpu, const uint8_t)
  {
    cpu.wait();
    return 4; // T-states
  }
  PRINTER(HALT) (char* buffer, size_t len, CPU&, uint8_t) {
    return snprintf(buffer, len, "HALT");
  }

  DEF_INSTR(NOP);
  DEF_INSTR(LD_N_SP);
  DEF_INSTR(LD_R_N);
  DEF_INSTR(LD_R_A_R);
  DEF_INSTR(LD_D_N);
  DEF_INSTR(LD_D_D);
  DEF_INSTR(LD_N_A_N);
  DEF_INSTR(RST);
  DEF_INSTR(HALT);
  DEF_INSTR(STOP);
  DEF_INSTR(JP);
  DEF_INSTR(CALL);
  DEF_INSTR(MISSING);
}
