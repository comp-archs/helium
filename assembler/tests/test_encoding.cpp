#include "encoding.hpp"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

using helium::asm_isa::EncodingError;
using helium::asm_isa::encodeI;
using helium::asm_isa::encodeJ;
using helium::asm_isa::encodeR;
using helium::asm_isa::fieldFunct;
using helium::asm_isa::fieldIImm;
using helium::asm_isa::fieldJTarget;
using helium::asm_isa::fieldOpcode;
using helium::asm_isa::fieldRd;
using helium::asm_isa::fieldRs1;
using helium::asm_isa::fieldRs2;

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
    // 20-bit signed range: [-524288, 524287]
    const std::int32_t minImm = -(1 << 19);
    const std::int32_t maxImm =  (1 << 19) - 1;

    const std::uint32_t wMin = encodeI(0x8, 0, 0, minImm);
    const std::uint32_t wMax = encodeI(0x8, 0, 0, maxImm);

    expectEqI32(fieldIImm(wMin), minImm, "I-type min imm roundtrip");
    expectEqI32(fieldIImm(wMax), maxImm, "I-type max imm roundtrip");

    expectThrows([&]() { (void)encodeI(0x8, 0, 0, minImm - 1); }, "I-type imm underflow throws");
    expectThrows([&]() { (void)encodeI(0x8, 0, 0, maxImm + 1); }, "I-type imm overflow throws");
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

} // namespace

int main() {
    testEncodeRBasic();
    testEncodeIBasicAndSign();
    testEncodeIBounds20Bit();
    testEncodeJBasicAndBounds();
    testFieldRangeChecks();

    if (g_failures == 0) {
        std::cout << "[PASS] test_encoding\n";
        return 0;
    }

    std::cerr << "[FAIL] test_encoding: " << g_failures << " failure(s)\n";
    return 1;
}