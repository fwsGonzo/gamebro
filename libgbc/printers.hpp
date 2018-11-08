#pragma once
#include <cstdint>

static const char* cstr_reg_bc_de(const uint8_t bf) {
  if (bf) return "DE";
  return "BC";
}
static const char* cstr_reg(const uint8_t bf, bool sp) {
  static const char* notsp[] = {"BC", "DE", "HL", "AF"};
  static const char* notaf[] = {"BC", "DE", "HL", "SP"};
  if (sp) return notaf[bf >> 4];
  return notsp[bf >> 4];
}
static const char* cstr_dest(const uint8_t bf) {
  static const char* dest[] = {"B", "C", "D", "E", "H", "L", "(HL)", "A"};
  return dest[bf & 0x7];
}
static const char* cstr_flag(const uint8_t bf) {
  static const char* s[] = {"not zero", "zero", "not carry", "carry"};
  return s[(bf >> 3) & 0x3];
}
static const char* cstr_flag_en(const uint8_t bf, const uint8_t flags) {
  const uint8_t f = (bf >> 3) & 0x3;
  return (flags & (1 << (4 + f))) ? "YES" : "NO";
}
static void fill_flag_buffer(char* buffer, size_t len,
                             const uint8_t opcode, const uint8_t flags)
{
  snprintf(buffer, len, "%s: %s",
           cstr_flag(opcode), cstr_flag_en(opcode, flags));
}
