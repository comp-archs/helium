#include "isa.hpp"

#include <array>
#include <cctype>
#include <string>
#include <string_view>

namespace helium {
namespace asm_isa {
namespace {

using namespace std::literals;

// Canonical instruction and assembler-pseudo table from ISA v0.1.
constexpr std::array<InstructionInfo, 23> kInstructions{{
    {"NOP"sv,  Opcode::ALU,  Format::R, static_cast<uint8_t>(AluFunction::NOP),  OperandForm::None,     false},
    {"ADD"sv,  Opcode::ALU,  Format::R, static_cast<uint8_t>(AluFunction::ADD),  OperandForm::RRR,      false},
    {"SUB"sv,  Opcode::ALU,  Format::R, static_cast<uint8_t>(AluFunction::SUB),  OperandForm::RRR,      false},
    {"AND"sv,  Opcode::ALU,  Format::R, static_cast<uint8_t>(AluFunction::AND),  OperandForm::RRR,      false},
    {"OR"sv,   Opcode::ALU,  Format::R, static_cast<uint8_t>(AluFunction::OR),   OperandForm::RRR,      false},
    {"XOR"sv,  Opcode::ALU,  Format::R, static_cast<uint8_t>(AluFunction::XOR),  OperandForm::RRR,      false},
    {"SHL"sv,  Opcode::ALU,  Format::R, static_cast<uint8_t>(AluFunction::SHL),  OperandForm::RRR,      false},
    {"SHR"sv,  Opcode::ALU,  Format::R, static_cast<uint8_t>(AluFunction::SHR),  OperandForm::RRR,      false},
    {"SAR"sv,  Opcode::ALU,  Format::R, static_cast<uint8_t>(AluFunction::SAR),  OperandForm::RRR,      false},
    {"SLT"sv,  Opcode::ALU,  Format::R, static_cast<uint8_t>(AluFunction::SLT),  OperandForm::RRR,      false},
    {"SLTU"sv, Opcode::ALU,  Format::R, static_cast<uint8_t>(AluFunction::SLTU), OperandForm::RRR,      false},
    {"ADDI"sv, Opcode::ADDI, Format::I, 0,                                       OperandForm::RRI,      false},
    {"LUI"sv,  Opcode::LUI,  Format::I, 0,                                       OperandForm::RI,       false},
    {"LD"sv,   Opcode::LD,   Format::I, 0,                                       OperandForm::RRI,      false},
    {"ST"sv,   Opcode::ST,   Format::I, 0,                                       OperandForm::RRI,      false},
    {"BEQ"sv,  Opcode::BEQ,  Format::I, 0,                                       OperandForm::RRTarget, false},
    {"BNE"sv,  Opcode::BNE,  Format::I, 0,                                       OperandForm::RRTarget, false},
    {"JMP"sv,  Opcode::JMP,  Format::J, 0,                                       OperandForm::Target,   false},
    {"JAL"sv,  Opcode::JAL,  Format::J, 0,                                       OperandForm::Target,   false},
    {"JALR"sv, Opcode::JALR, Format::I, 0,                                       OperandForm::RRI,      false},
    {"HALT"sv, Opcode::HALT, Format::J, 0,                                       OperandForm::None,     false},
    {"LDI"sv,  Opcode::ADDI, Format::I, 0,                                       OperandForm::RI,       true},
    {"RET"sv,  Opcode::JALR, Format::I, 0,                                       OperandForm::None,     true},
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

    for (std::size_t index = 0; index < kRegNames.size(); ++index) {
        if (key == kRegNames[index]) {
            return static_cast<uint8_t>(index);
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

bool isLegalInstruction(std::uint32_t word) {
    const auto opcode = static_cast<Opcode>((word >> 28u) & 0xFu);

    switch (opcode) {
        case Opcode::ALU: {
            const std::uint8_t funct = static_cast<std::uint8_t>(word & 0xFu);
            if ((word & 0x0000FFF0u) != 0 || funct > static_cast<std::uint8_t>(AluFunction::SLTU)) {
                return false;
            }
            return funct != static_cast<std::uint8_t>(AluFunction::NOP) || word == 0;
        }
        case Opcode::LUI:
            return (word & 0x00F00000u) == 0;
        case Opcode::HALT:
            return (word & 0x0FFFFFFFu) == 0;
        case Opcode::ADDI:
        case Opcode::LD:
        case Opcode::ST:
        case Opcode::BEQ:
        case Opcode::BNE:
        case Opcode::JMP:
        case Opcode::JAL:
        case Opcode::JALR:
            return true;
        case Opcode::RESERVED_1:
        case Opcode::RESERVED_C:
        case Opcode::RESERVED_D:
        case Opcode::RESERVED_E:
        case Opcode::RESERVED_F:
            return false;
    }
    return false;
}

std::string_view toString(Format f) {
    switch (f) {
        case Format::R: return "R";
        case Format::I: return "I";
        case Format::J: return "J";
    }
    return "Unknown";
}

std::string_view toString(OperandForm form) {
    switch (form) {
        case OperandForm::None: return "None";
        case OperandForm::RRR: return "RRR";
        case OperandForm::RRI: return "RRI";
        case OperandForm::RI: return "RI";
        case OperandForm::RRTarget: return "RRTarget";
        case OperandForm::Target: return "Target";
    }
    return "Unknown";
}

std::string_view toString(Opcode op) {
    switch (op) {
        case Opcode::ALU: return "ALU";
        case Opcode::RESERVED_1: return "RESERVED";
        case Opcode::ADDI: return "ADDI";
        case Opcode::LUI: return "LUI";
        case Opcode::LD:  return "LD";
        case Opcode::ST:  return "ST";
        case Opcode::BEQ: return "BEQ";
        case Opcode::BNE: return "BNE";
        case Opcode::JMP: return "JMP";
        case Opcode::JAL: return "JAL";
        case Opcode::JALR: return "JALR";
        case Opcode::HALT: return "HALT";
        case Opcode::RESERVED_C:
        case Opcode::RESERVED_D:
        case Opcode::RESERVED_E:
        case Opcode::RESERVED_F:
            return "RESERVED";
    }
    return "Unknown";
}

std::string_view toString(AluFunction funct) {
    switch (funct) {
        case AluFunction::NOP: return "NOP";
        case AluFunction::ADD: return "ADD";
        case AluFunction::SUB: return "SUB";
        case AluFunction::AND: return "AND";
        case AluFunction::OR: return "OR";
        case AluFunction::XOR: return "XOR";
        case AluFunction::SHL: return "SHL";
        case AluFunction::SHR: return "SHR";
        case AluFunction::SAR: return "SAR";
        case AluFunction::SLT: return "SLT";
        case AluFunction::SLTU: return "SLTU";
    }
    return "Unknown";
}

} // namespace asm_isa
} // namespace helium
