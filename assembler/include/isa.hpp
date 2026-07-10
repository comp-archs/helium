#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace helium {
namespace asm_isa {

enum class Format : uint8_t { R, I, J };

enum class OperandForm : uint8_t {
  None,
  RRR,
  RRI,
  RI,
  RRTarget,
  Target
};

enum class Opcode : uint8_t {
  ALU = 0x0,
  RESERVED_1 = 0x1,
  ADDI = 0x2,
  LUI = 0x3,
  LD = 0x4,
  ST = 0x5,
  BEQ = 0x6,
  BNE = 0x7,
  JMP = 0x8,
  JAL = 0x9,
  JALR = 0xA,
  HALT = 0xB,
  RESERVED_C = 0xC,
  RESERVED_D = 0xD,
  RESERVED_E = 0xE,
  RESERVED_F = 0xF
};

enum class AluFunction : uint8_t {
  NOP = 0x0,
  ADD = 0x1,
  SUB = 0x2,
  AND = 0x3,
  OR = 0x4,
  XOR = 0x5,
  SHL = 0x6,
  SHR = 0x7,
  SAR = 0x8,
  SLT = 0x9,
  SLTU = 0xA
};

struct InstructionInfo {
  std::string_view mnemonic;
  Opcode opcode;
  Format format;
  uint8_t funct;
  OperandForm operands;
  bool pseudo;
};

struct RegisterInfo {
  std::string_view name;
  uint8_t index;
};


std::optional<InstructionInfo> lookupInstruction(std::string_view mnemonic);

bool isValidMnemonic(std::string_view mnemonic);

std::optional<uint8_t> parseRegister(std::string_view token);

std::optional<std::string_view> registerName(uint8_t index);

bool isLegalInstruction(std::uint32_t word);

std::string_view toString(Format f);
std::string_view toString(OperandForm form);
std::string_view toString(Opcode op);
std::string_view toString(AluFunction funct);

std::string toUpperAscii(std::string_view s);
} // namespace asm_isa
} // namespace helium
