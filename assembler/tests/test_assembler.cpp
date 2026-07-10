#include "assembler.hpp"
#include "parser.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <string>
#include <vector>

using namespace helium::assembler;
int main() {
    int failures = 0;
    const std::string golden = "NOP\nADD R1,R2,R3\nSUB R1,R2,R3\nAND R1,R2,R3\nOR R1,R2,R3\nXOR R1,R2,R3\nSHL R1,R2,R3\nSHR R1,R2,R3\nSAR R1,R2,R3\nSLT R1,R2,R3\nSLTU R1,R2,R3\nADDI R1,R2,#-1\nLUI R1,#0xABCDE\nLD R1,R2,#16\nST R1,R2,#-16\nBEQ R1,R2,#16\nBNE R1,R2,#-4\nJMP #16\nJAL #-4\nJALR R1,R2,#-4\nHALT\nLDI R1,#-1\nRET\n";
    const std::array<std::uint32_t, 23> expected{{0,0x01230001,0x01230002,0x01230003,0x01230004,0x01230005,0x01230006,0x01230007,0x01230008,0x01230009,0x0123000A,0x212FFFFF,0x310ABCDE,0x41200010,0x512FFFF0,0x61200010,0x712FFFFC,0x80000010,0x9FFFFFFC,0xA12FFFFC,0xB0000000,0x210FFFFF,0xA0D00000}};
    const auto result = assemble(golden);
    if (result.words.size() != expected.size() || !std::equal(result.words.begin(), result.words.end(), expected.begin()) || result.bytes.size() != expected.size() * 4) ++failures;
    if (assemble("ADD R1,R2,R3").bytes != std::vector<std::uint8_t>{0x01, 0x00, 0x23, 0x01}) ++failures;
    const auto labels = assemble("Top: NOP\n bEq zero,R0,End\n JMP Top\nEnd: LDI sp,#-0x80000\nJALR R0,lr,#3\n");
    if (labels.words[1] != 0x60000008 || labels.words[2] != 0x8FFFFFF8 || labels.words[3] != 0x2F080000 || labels.words[4] != 0xA0D00003) ++failures;
    for (const char* ok : {"ADDI R1,R0,#524287", "ADDI R1,R0,#-524288", "LUI R1,#0xfffff", "JMP #134217724", "JMP #-134217728"})
        try { (void)assemble(ok); } catch (...) { ++failures; }
    for (const char* bad : {"x: NOP\nx: HALT", "JMP absent", "ADD R1,R2", "ADD R1,R2,#1", "ADDI R1,R2,3", "ADDI R1,R2,#524288", "LUI R1,#-1", "LUI R1,#0x100000", "BEQ R1,R2,#2", "JMP #-2", "JMP #134217728"})
        try { (void)assemble(bad); ++failures; } catch (const SourceError&) {}
    for (const char* badRegister : {"ADD R01,R2,R3", "ADD R00,R2,R3", "ADD R999999999999999999999999999999999999,R2,R3"})
        try { (void)assemble(badRegister); ++failures; } catch (const SourceError&) {}
    if (failures) { std::cerr << "FAIL test_assembler: " << failures << '\n'; return 1; }
    std::cout << "PASS test_assembler\n"; return 0;
}
