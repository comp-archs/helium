#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace helium {
namespace asm_isa {

enum class Format : uint8_t { R, I, J };

enum class Opcode : uint8_t {
  NOP = 0x0,
  ADD = 0x1,
  SUB = 0x2,
  AND = 0x3,
  OR = 0x4,
  XOR = 0x5,
  SHL = 0x6,
  SHR = 0x7,
  LDI = 0x8,
  LD = 0x9,
  ST = 0xA,
  BEQ = 0xB,
  BNE = 0xC,
  JMP = 0xD,
  JAL = 0xE,
  RESERVED = 0xF
};

struct InstructionInfo {
  std::string_view mnemonic;
  Opcode opcode;
  Format format;
  uint8_t funct;
};

struct RegisterInfo {
  std::string_view name;
  uint8_t index;
};


std::optional<InstructionInfo> lookupInstruction(std::string_view mnemonic);

bool isValidMnemonic(std::string_view mnemonic);

std::optional<uint8_t> parseRegister(std::string_view token);

std::optional<std::string_view> registerName(uint8_t index);

std::string_view toString(Format f);
std::string_view toString(Opcode op);

std::string toUpperAscii(std::string_view s);
}
