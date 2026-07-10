#include "isa.hpp"

#include <array>
#include <cctype>
#include <string>
#include <string_view>

namespace helium {
namespace asm_isa {
namespace {

using namespace std::literals;

// Canonical instruction table from the ISA spec.
// funct is currently 0 for all ops; refine later if your spec assigns funct values.
constexpr std::array<InstructionInfo, 15> kInstructions{{
    {"NOP"sv, Opcode::NOP, Format::J, 0},
    {"ADD"sv, Opcode::ADD, Format::R, 0},
    {"SUB"sv, Opcode::SUB, Format::R, 0},
    {"AND"sv, Opcode::AND, Format::R, 0},
    {"OR"sv,  Opcode::OR,  Format::R, 0},
    {"XOR"sv, Opcode::XOR, Format::R, 0},
    {"SHL"sv, Opcode::SHL, Format::R, 0},
    {"SHR"sv, Opcode::SHR, Format::R, 0},
    {"LDI"sv, Opcode::LDI, Format::I, 0},
    {"LD"sv,  Opcode::LD,  Format::I, 0},
    {"ST"sv,  Opcode::ST,  Format::I, 0},
    {"BEQ"sv, Opcode::BEQ, Format::I, 0},
    {"BNE"sv, Opcode::BNE, Format::I, 0},
    {"JMP"sv, Opcode::JMP, Format::J, 0},
    {"JAL"sv, Opcode::JAL, Format::J, 0},
}};

// R0..R15 canonical names
constexpr std::array<std::string_view, 16> kRegNames{{
    "R0","R1","R2","R3","R4","R5","R6","R7",
    "R8","R9","R10","R11","R12","R13","R14","R15"
}};

} // namespace

std::string toUpperAscii(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (unsigned char ch : s) {
        out.push_back(static_cast<char>(std::toupper(ch)));
    }
    return out;
}

std::optional<InstructionInfo> lookupInstruction(std::string_view mnemonic) {
    const std::string key = toUpperAscii(mnemonic);
    for (const auto& ins : kInstructions) {
        if (ins.mnemonic == key) {
            return ins;
        }
    }
    return std::nullopt;
}

bool isValidMnemonic(std::string_view mnemonic) {
    return lookupInstruction(mnemonic).has_value();
}

std::optional<uint8_t> parseRegister(std::string_view token) {
    const std::string key = toUpperAscii(token);

    // Aliases
    if (key == "ZERO") return static_cast<uint8_t>(0);   // R0
    if (key == "LR")   return static_cast<uint8_t>(13);  // R13
    if (key == "FP")   return static_cast<uint8_t>(14);  // R14
    if (key == "SP")   return static_cast<uint8_t>(15);  // R15

    // Canonical R0..R15
    if (key.size() >= 2 && key[0] == 'R') {
        int value = 0;
        for (size_t i = 1; i < key.size(); ++i) {
            if (!std::isdigit(static_cast<unsigned char>(key[i]))) {
                return std::nullopt;
            }
            value = value * 10 + (key[i] - '0');
        }
        if (value >= 0 && value <= 15) {
            return static_cast<uint8_t>(value);
        }
    }

    return std::nullopt;
}

std::optional<std::string_view> registerName(uint8_t index) {
    if (index < kRegNames.size()) {
        return kRegNames[index];
    }
    return std::nullopt;
}

std::string_view toString(Format f) {
    switch (f) {
        case Format::R: return "R";
        case Format::I: return "I";
        case Format::J: return "J";
    }
    return "Unknown";
}

std::string_view toString(Opcode op) {
    switch (op) {
        case Opcode::NOP: return "NOP";
        case Opcode::ADD: return "ADD";
        case Opcode::SUB: return "SUB";
        case Opcode::AND: return "AND";
        case Opcode::OR:  return "OR";
        case Opcode::XOR: return "XOR";
        case Opcode::SHL: return "SHL";
        case Opcode::SHR: return "SHR";
        case Opcode::LDI: return "LDI";
        case Opcode::LD:  return "LD";
        case Opcode::ST:  return "ST";
        case Opcode::BEQ: return "BEQ";
        case Opcode::BNE: return "BNE";
        case Opcode::JMP: return "JMP";
        case Opcode::JAL: return "JAL";
        case Opcode::RESERVED: return "RESERVED";
    }
    return "Unknown";
}

} // namespace asm_isa
} // namespace helium
