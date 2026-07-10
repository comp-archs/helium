#include "encoding.hpp"
#include "isa.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

using helium::asm_isa::AluFunction;
using helium::asm_isa::EncodingError;
using helium::asm_isa::Format;
using helium::asm_isa::Opcode;
using helium::asm_isa::OperandForm;
using helium::asm_isa::encodeI;
using helium::asm_isa::encodeIU;
using helium::asm_isa::encodeJ;
using helium::asm_isa::encodeR;
using helium::asm_isa::fieldFunct;
using helium::asm_isa::fieldIImm;
using helium::asm_isa::fieldIImmRaw;
using helium::asm_isa::fieldJTarget;
using helium::asm_isa::fieldOpcode;
using helium::asm_isa::fieldRd;
using helium::asm_isa::fieldRs1;
using helium::asm_isa::fieldRs2;
using helium::asm_isa::isLegalInstruction;
using helium::asm_isa::lookupInstruction;

namespace {

int g_failures = 0;

void expectTrue(bool cond, const std::string& msg) {
    if (!cond) {
        ++g_failures;
        std::cerr << "[FAIL] " << msg << "\n";
    }
}

void expectEqU32(std::uint32_t actual, std::uint32_t expected, const std::string& msg) {
    if (actual != expected) {
        ++g_failures;
        std::cerr << "[FAIL] " << msg
                  << " expected=0x" << std::hex << expected
                  << " actual=0x" << actual << std::dec << "\n";
    }
}

void expectEqI32(std::int32_t actual, std::int32_t expected, const std::string& msg) {
    if (actual != expected) {
        ++g_failures;
        std::cerr << "[FAIL] " << msg
                  << " expected=" << expected
                  << " actual=" << actual << "\n";
    }
}

template <typename Fn>
void expectThrows(Fn&& fn, const std::string& msg) {
    try {
        fn();
        ++g_failures;
        std::cerr << "[FAIL] " << msg << " (expected throw)\n";
    } catch (const EncodingError&) {
        // pass
    } catch (...) {
        ++g_failures;
        std::cerr << "[FAIL] " << msg << " (threw wrong exception type)\n";
    }
}

void testEncodeRBasic() {
    // opcode=1, rd=2, rs1=3, rs2=4, funct=5
    // word = (1<<28)|(2<<24)|(3<<20)|(4<<16)|5 = 0x12340005
    const std::uint32_t w = encodeR(0x1, 0x2, 0x3, 0x4, 0x5);
    expectEqU32(w, 0x12340005u, "encodeR basic packing");
    expectEqU32(fieldOpcode(w), 0x1, "fieldOpcode from R");
    expectEqU32(fieldRd(w),     0x2, "fieldRd from R");
    expectEqU32(fieldRs1(w),    0x3, "fieldRs1 from R");
    expectEqU32(fieldRs2(w),    0x4, "fieldRs2 from R");
    expectEqU32(fieldFunct(w),  0x5, "fieldFunct from R");
}

void testEncodeIBasicAndSign() {
    // I_IMM_BITS assumed 20 in encoding.hpp
    // opcode=8, rd=1, rs1=2, imm=-1 => imm field = 0xFFFFF
    // word = (8<<28)|(1<<24)|(2<<20)|0xFFFFF = 0x812FFFFF
    const std::uint32_t w = encodeI(0x8, 0x1, 0x2, -1);
    expectEqU32(w, 0x812FFFFFu, "encodeI basic packing with -1");
    expectEqU32(fieldOpcode(w), 0x8, "fieldOpcode from I");
    expectEqU32(fieldRd(w),     0x1, "fieldRd from I");
    expectEqU32(fieldRs1(w),    0x2, "fieldRs1 from I");
    expectEqI32(fieldIImm(w),   -1,  "fieldIImm sign extension");
}

void testEncodeIBounds20Bit() {
    const std::int32_t minImm = -(1 << 19);
    const std::int32_t maxImm =  (1 << 19) - 1;

    const std::uint32_t wMin = encodeI(0x8, 0, 0, minImm);
    const std::uint32_t wMax = encodeI(0x8, 0, 0, maxImm);

    expectEqI32(fieldIImm(wMin), minImm, "I-type min imm roundtrip");
    expectEqI32(fieldIImm(wMax), maxImm, "I-type max imm roundtrip");

    expectThrows([&]() { (void)encodeI(0x8, 0, 0, minImm - 1); }, "I-type imm underflow throws");
    expectThrows([&]() { (void)encodeI(0x8, 0, 0, maxImm + 1); }, "I-type imm overflow throws");
}

void testEncodeIUnsigned() {
    const std::uint32_t word = encodeIU(0x3, 1, 0, 0xFFFFFu);
    expectEqU32(word, 0x310FFFFFu, "encodeIU packs raw 20-bit immediate");
    expectEqU32(fieldIImmRaw(word), 0xFFFFFu, "fieldIImmRaw preserves unsigned value");
    expectThrows([]() { (void)encodeIU(0x3, 0, 0, 0x100000u); },
                 "unsigned I-type immediate overflow throws");
}

void testEncodeJBasicAndBounds() {
    // opcode=0xD, target=-1 => low 28 bits all ones
    // word = (0xD<<28)|0x0FFFFFFF = 0xDFFFFFFF
    const std::uint32_t w = encodeJ(0xD, -1);
    expectEqU32(w, 0xDFFFFFFFu, "encodeJ basic packing with -1");
    expectEqU32(fieldOpcode(w), 0xD, "fieldOpcode from J");
    expectEqI32(fieldJTarget(w), -1, "fieldJTarget sign extension");

    // 28-bit signed range: [-134217728, 134217727]
    const std::int32_t minT = -(1 << 27);
    const std::int32_t maxT =  (1 << 27) - 1;

    const std::uint32_t wMin = encodeJ(0xD, minT);
    const std::uint32_t wMax = encodeJ(0xD, maxT);

    expectEqI32(fieldJTarget(wMin), minT, "J-type min target roundtrip");
    expectEqI32(fieldJTarget(wMax), maxT, "J-type max target roundtrip");

    expectThrows([&]() { (void)encodeJ(0xD, minT - 1); }, "J-type target underflow throws");
    expectThrows([&]() { (void)encodeJ(0xD, maxT + 1); }, "J-type target overflow throws");
}

void testFieldRangeChecks() {
    expectThrows([&]() { (void)encodeR(16, 0, 0, 0, 0); }, "opcode > 4 bits throws");
    expectThrows([&]() { (void)encodeR(0, 16, 0, 0, 0); }, "rd > 4 bits throws");
    expectThrows([&]() { (void)encodeR(0, 0, 16, 0, 0); }, "rs1 > 4 bits throws");
    expectThrows([&]() { (void)encodeR(0, 0, 0, 16, 0); }, "rs2 > 4 bits throws");
    expectThrows([&]() { (void)encodeR(0, 0, 0, 0, 16); }, "funct > 4 bits throws");
}

constexpr std::uint8_t value(Opcode opcode) {
    return static_cast<std::uint8_t>(opcode);
}

constexpr std::uint8_t value(AluFunction funct) {
    return static_cast<std::uint8_t>(funct);
}

void testIsaGoldenVectors() {
    struct Vector {
        const char* name;
        std::uint32_t actual;
        std::uint32_t expected;
    };

    const std::array<Vector, 21> vectors{{
        {"NOP",  encodeR(value(Opcode::ALU), 0, 0, 0, value(AluFunction::NOP)),  0x00000000u},
        {"ADD",  encodeR(value(Opcode::ALU), 1, 2, 3, value(AluFunction::ADD)),  0x01230001u},
        {"SUB",  encodeR(value(Opcode::ALU), 1, 2, 3, value(AluFunction::SUB)),  0x01230002u},
        {"AND",  encodeR(value(Opcode::ALU), 1, 2, 3, value(AluFunction::AND)),  0x01230003u},
        {"OR",   encodeR(value(Opcode::ALU), 1, 2, 3, value(AluFunction::OR)),   0x01230004u},
        {"XOR",  encodeR(value(Opcode::ALU), 1, 2, 3, value(AluFunction::XOR)),  0x01230005u},
        {"SHL",  encodeR(value(Opcode::ALU), 1, 2, 3, value(AluFunction::SHL)),  0x01230006u},
        {"SHR",  encodeR(value(Opcode::ALU), 1, 2, 3, value(AluFunction::SHR)),  0x01230007u},
        {"SAR",  encodeR(value(Opcode::ALU), 1, 2, 3, value(AluFunction::SAR)),  0x01230008u},
        {"SLT",  encodeR(value(Opcode::ALU), 1, 2, 3, value(AluFunction::SLT)),  0x01230009u},
        {"SLTU", encodeR(value(Opcode::ALU), 1, 2, 3, value(AluFunction::SLTU)), 0x0123000Au},
        {"ADDI", encodeI(value(Opcode::ADDI), 1, 2, -1),                         0x212FFFFFu},
        {"LUI",  encodeIU(value(Opcode::LUI), 1, 0, 0xABCDEu),                   0x310ABCDEu},
        {"LD",   encodeI(value(Opcode::LD), 1, 2, 16),                            0x41200010u},
        {"ST",   encodeI(value(Opcode::ST), 1, 2, -16),                           0x512FFFF0u},
        {"BEQ",  encodeI(value(Opcode::BEQ), 1, 2, 16),                           0x61200010u},
        {"BNE",  encodeI(value(Opcode::BNE), 1, 2, -4),                           0x712FFFFCu},
        {"JMP",  encodeJ(value(Opcode::JMP), 16),                                 0x80000010u},
        {"JAL",  encodeJ(value(Opcode::JAL), -4),                                 0x9FFFFFFCu},
        {"JALR", encodeI(value(Opcode::JALR), 1, 2, -4),                          0xA12FFFFCu},
        {"HALT", encodeJ(value(Opcode::HALT), 0),                                 0xB0000000u},
    }};

    for (const auto& vector : vectors) {
        expectEqU32(vector.actual, vector.expected, std::string("golden vector ") + vector.name);
        expectTrue(isLegalInstruction(vector.expected), std::string("legal golden vector ") + vector.name);
    }

    expectEqU32(encodeI(value(Opcode::ADDI), 1, 0, -1), 0x210FFFFFu,
                "LDI pseudo expands to ADDI rd, R0, imm");
    expectEqU32(encodeI(value(Opcode::JALR), 0, 13, 0), 0xA0D00000u,
                "RET pseudo expands to JALR R0, LR, 0");
}

void testLegalEncodingRules() {
    expectTrue(!isLegalInstruction(0x10000000u), "reserved primary opcode is illegal");
    expectTrue(!isLegalInstruction(0xC0000000u), "reserved high primary opcode is illegal");
    expectTrue(!isLegalInstruction(0x0000000Bu), "reserved ALU function is illegal");
    expectTrue(!isLegalInstruction(0x00000011u), "nonzero R reserved bits are illegal");
    expectTrue(!isLegalInstruction(0x01000000u), "NOP with operands is illegal");
    expectTrue(!isLegalInstruction(0x31100000u), "LUI with nonzero rs1 is illegal");
    expectTrue(!isLegalInstruction(0xB0000001u), "HALT payload is illegal");
    expectTrue(isLegalInstruction(0x60000002u),
               "misaligned branch target is an execution fault, not an illegal encoding");
}

void testInstructionMetadata() {
    const auto nop = lookupInstruction("nop");
    expectTrue(nop.has_value(), "NOP metadata exists");
    expectTrue(nop && nop->format == Format::R && nop->operands == OperandForm::None && !nop->pseudo,
               "NOP is a zero-operand physical R encoding");

    const auto ldi = lookupInstruction("LDI");
    expectTrue(ldi && ldi->opcode == Opcode::ADDI && ldi->pseudo,
               "LDI is an ADDI pseudo-instruction");

    const auto ret = lookupInstruction("RET");
    expectTrue(ret && ret->opcode == Opcode::JALR && ret->pseudo,
               "RET is a JALR pseudo-instruction");
}

} // namespace

int main() {
    testEncodeRBasic();
    testEncodeIBasicAndSign();
    testEncodeIBounds20Bit();
    testEncodeIUnsigned();
    testEncodeJBasicAndBounds();
    testFieldRangeChecks();
    testIsaGoldenVectors();
    testLegalEncodingRules();
    testInstructionMetadata();

    if (g_failures == 0) {
        std::cout << "PASS test_encoding\n";
        return 0;
    }

    std::cerr << "FAIL test_encoding: " << g_failures << " failure(s)\n";
    return 1;
}
