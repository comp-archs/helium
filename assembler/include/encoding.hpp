#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

namespace helium {
namespace asm_isa {

// Thrown when an instruction field is out of range.
class EncodingError : public std::runtime_error {
public:
    explicit EncodingError(const std::string& message)
        : std::runtime_error(message) {}
};


// Bit-width constants
constexpr std::uint32_t OPCODE_BITS = 4;
constexpr std::uint32_t REG_BITS    = 4;
constexpr std::uint32_t FUNCT_BITS  = 4;

constexpr std::uint32_t I_IMM_BITS  = 20;
constexpr std::uint32_t J_TGT_BITS  = 28;

// Range helpers
bool fitsUnsigned(std::int64_t value, std::uint32_t bits);
bool fitsSigned(std::int64_t value, std::uint32_t bits);

// Primitive field packers
// Validate field widths and pack 32-bit words. They can represent reserved or
// noncanonical encodings; assembly output must also pass isLegalInstruction.
//
// R-type layout:
// [31:28]=opcode, [27:24]=rd, [23:20]=rs1, [19:16]=rs2, [15:4]=0, [3:0]=funct
std::uint32_t encodeR(std::uint8_t opcode,
                      std::uint8_t rd,
                      std::uint8_t rs1,
                      std::uint8_t rs2,
                      std::uint8_t funct = 0);

// I-type layout (current interpretation from diagram):
// [31:28]=opcode, [27:24]=rd, [23:20]=rs1, [19:0]=imm (two's complement signed)
std::uint32_t encodeI(std::uint8_t opcode,
                      std::uint8_t rd,
                      std::uint8_t rs1,
                      std::int32_t imm);

// I-type layout with an unsigned raw immediate, used by LUI.
std::uint32_t encodeIU(std::uint8_t opcode,
                       std::uint8_t rd,
                       std::uint8_t rs1,
                       std::uint32_t imm);

// J-type layout:
// [31:28]=opcode, [27:0]=target (signed PC-relative immediate field)
std::uint32_t encodeJ(std::uint8_t opcode,
                      std::int32_t target);

// Utility extractors (handy for tests/debug)
std::uint8_t  fieldOpcode(std::uint32_t word);
std::uint8_t  fieldRd(std::uint32_t word);
std::uint8_t  fieldRs1(std::uint32_t word);
std::uint8_t  fieldRs2(std::uint32_t word);
std::uint8_t  fieldFunct(std::uint32_t word);
std::uint32_t fieldIImmRaw(std::uint32_t word);
std::int32_t  fieldIImm(std::uint32_t word);   // sign-extended
std::int32_t  fieldJTarget(std::uint32_t word); // sign-extended

} // namespace asm_isa
} // namespace helium
