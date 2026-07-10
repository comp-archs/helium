#include "encoding.hpp"

#include <limits>
#include <sstream>

namespace helium {
namespace asm_isa {
namespace {

[[noreturn]] void throwRange(const char* field,
                             std::int64_t value,
                             std::uint32_t bits,
                             bool isSigned) {
    std::ostringstream oss;
    oss << "Encoding error: field '" << field << "' value " << value
        << " does not fit in " << bits << "-bit "
        << (isSigned ? "signed" : "unsigned") << " range";
    throw EncodingError(oss.str());
}

constexpr std::uint32_t maskFor(std::uint32_t bits) {
    return (bits >= 32u) ? 0xFFFFFFFFu : ((1u << bits) - 1u);
}

std::uint32_t packSigned(std::int64_t value, std::uint32_t bits, const char* field) {
    if (!fitsSigned(value, bits)) {
        throwRange(field, value, bits, true);
    }
    return static_cast<std::uint32_t>(value) & maskFor(bits);
}

std::uint32_t packUnsigned(std::int64_t value, std::uint32_t bits, const char* field) {
    if (!fitsUnsigned(value, bits)) {
        throwRange(field, value, bits, false);
    }
    return static_cast<std::uint32_t>(value);
}

std::int32_t signExtend(std::uint32_t raw, std::uint32_t bits) {
    if (bits == 0 || bits >= 32) {
        return static_cast<std::int32_t>(raw);
    }
    const std::uint32_t signBit = 1u << (bits - 1u);
    const std::uint32_t fullMask = 0xFFFFFFFFu;
    if (raw & signBit) {
        raw |= (fullMask << bits);
    }
    return static_cast<std::int32_t>(raw);
}

} // namespace

bool fitsUnsigned(std::int64_t value, std::uint32_t bits) {
    if (bits == 0 || bits > 63) return false;
    if (value < 0) return false;
    const std::uint64_t maxv = (1ull << bits) - 1ull;
    return static_cast<std::uint64_t>(value) <= maxv;
}

bool fitsSigned(std::int64_t value, std::uint32_t bits) {
    if (bits == 0 || bits > 63) return false;
    const std::int64_t minv = -(1ll << (bits - 1u));
    const std::int64_t maxv =  (1ll << (bits - 1u)) - 1ll;
    return value >= minv && value <= maxv;
}

std::uint32_t encodeR(std::uint8_t opcode,
                      std::uint8_t rd,
                      std::uint8_t rs1,
                      std::uint8_t rs2,
                      std::uint8_t funct) {
    const std::uint32_t op   = packUnsigned(opcode, OPCODE_BITS, "opcode");
    const std::uint32_t d    = packUnsigned(rd,     REG_BITS,    "rd");
    const std::uint32_t s1   = packUnsigned(rs1,    REG_BITS,    "rs1");
    const std::uint32_t s2   = packUnsigned(rs2,    REG_BITS,    "rs2");
    const std::uint32_t fn   = packUnsigned(funct,  FUNCT_BITS,  "funct");

    // [31:28]=opcode, [27:24]=rd, [23:20]=rs1, [19:16]=rs2, [15:4]=0, [3:0]=funct
    return (op << 28u) | (d << 24u) | (s1 << 20u) | (s2 << 16u) | fn;
}

std::uint32_t encodeI(std::uint8_t opcode,
                      std::uint8_t rd,
                      std::uint8_t rs1,
                      std::int32_t imm) {
    const std::uint32_t op   = packUnsigned(opcode, OPCODE_BITS, "opcode");
    const std::uint32_t d    = packUnsigned(rd,     REG_BITS,    "rd");
    const std::uint32_t s1   = packUnsigned(rs1,    REG_BITS,    "rs1");
    const std::uint32_t im   = packSigned(imm,      I_IMM_BITS,  "imm");

    // [31:28]=opcode, [27:24]=rd, [23:20]=rs1, [19:0]=imm
    return (op << 28u) | (d << 24u) | (s1 << 20u) | im;
}

std::uint32_t encodeIU(std::uint8_t opcode,
                       std::uint8_t rd,
                       std::uint8_t rs1,
                       std::uint32_t imm) {
    const std::uint32_t op = packUnsigned(opcode, OPCODE_BITS, "opcode");
    const std::uint32_t d = packUnsigned(rd, REG_BITS, "rd");
    const std::uint32_t s1 = packUnsigned(rs1, REG_BITS, "rs1");
    const std::uint32_t im = packUnsigned(imm, I_IMM_BITS, "imm");

    return (op << 28u) | (d << 24u) | (s1 << 20u) | im;
}

std::uint32_t encodeJ(std::uint8_t opcode,
                      std::int32_t target) {
    const std::uint32_t op = packUnsigned(opcode, OPCODE_BITS, "opcode");
    const std::uint32_t tg = packSigned(target, J_TGT_BITS, "target");

    // [31:28]=opcode, [27:0]=target
    return (op << 28u) | tg;
}

std::uint8_t fieldOpcode(std::uint32_t word) {
    return static_cast<std::uint8_t>((word >> 28u) & maskFor(OPCODE_BITS));
}

std::uint8_t fieldRd(std::uint32_t word) {
    return static_cast<std::uint8_t>((word >> 24u) & maskFor(REG_BITS));
}

std::uint8_t fieldRs1(std::uint32_t word) {
    return static_cast<std::uint8_t>((word >> 20u) & maskFor(REG_BITS));
}

std::uint8_t fieldRs2(std::uint32_t word) {
    return static_cast<std::uint8_t>((word >> 16u) & maskFor(REG_BITS));
}

std::uint8_t fieldFunct(std::uint32_t word) {
    return static_cast<std::uint8_t>(word & maskFor(FUNCT_BITS));
}

std::uint32_t fieldIImmRaw(std::uint32_t word) {
    return word & maskFor(I_IMM_BITS);
}

std::int32_t fieldIImm(std::uint32_t word) {
    return signExtend(fieldIImmRaw(word), I_IMM_BITS);
}

std::int32_t fieldJTarget(std::uint32_t word) {
    const std::uint32_t raw = word & maskFor(J_TGT_BITS);
    return signExtend(raw, J_TGT_BITS);
}

} // namespace asm_isa
} // namespace helium
