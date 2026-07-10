#include "assembler.hpp"

#include "encoding.hpp"
#include "isa.hpp"
#include "parser.hpp"
#include "symbols.hpp"

#include <charconv>
#include <limits>
#include <string>

namespace helium::assembler {
namespace {
using namespace asm_isa;

[[noreturn]] void error(const Token& token, const std::string& message) { throw SourceError(token.location, message); }

std::int64_t number(const Token& token) {
    if (token.text.empty() || token.text[0] != '#') error(token, "numeric immediate must start with '#'");
    std::string_view value(token.text);
    value.remove_prefix(1);
    bool negative = false;
    if (!value.empty() && (value.front() == '-' || value.front() == '+')) {
        negative = value.front() == '-'; value.remove_prefix(1);
    }
    int base = 10;
    if (value.size() >= 2 && value[0] == '0' && (value[1] == 'x' || value[1] == 'X')) { base = 16; value.remove_prefix(2); }
    if (value.empty()) error(token, "invalid numeric immediate");
    std::uint64_t magnitude = 0;
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), magnitude, base);
    if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size() ||
        magnitude > (negative ? (std::uint64_t{1} << 63) : static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())))
        error(token, "invalid numeric immediate");
    if (negative && magnitude == (std::uint64_t{1} << 63)) return std::numeric_limits<std::int64_t>::min();
    return negative ? -static_cast<std::int64_t>(magnitude) : static_cast<std::int64_t>(magnitude);
}

std::uint8_t reg(const Token& token) {
    const auto result = parseRegister(token.text);
    if (!result) error(token, "expected register, got '" + token.text + "'");
    return *result;
}

void count(const ParsedLine& line, std::size_t expected) {
    if (line.operands.size() != expected)
        throw SourceError(line.mnemonic.location, "'" + line.mnemonic.text + "' expects " +
                          std::to_string(expected) + " operand(s), got " + std::to_string(line.operands.size()));
}

std::int64_t target(const Token& token, const SymbolTable& symbols, std::uint32_t pc) {
    if (!token.text.empty() && token.text[0] == '#') return number(token);
    if (!isValidIdentifier(token.text)) error(token, "expected label or numeric byte offset");
    return static_cast<std::int64_t>(symbols.resolve(token.text, token.location)) - static_cast<std::int64_t>(pc);
}

std::int32_t signedImm(const Token& token, unsigned bits) {
    const auto value = number(token);
    if (!fitsSigned(value, bits)) error(token, "immediate does not fit signed " + std::to_string(bits) + "-bit range");
    return static_cast<std::int32_t>(value);
}

std::uint32_t encode(const ParsedLine& line, const SymbolTable& symbols, std::uint32_t pc) {
    const auto info = lookupInstruction(line.mnemonic.text);
    if (!info) error(line.mnemonic, "unknown mnemonic '" + line.mnemonic.text + "'");
    const auto op = static_cast<std::uint8_t>(info->opcode);
    std::uint32_t word = 0;
    switch (info->operands) {
    case OperandForm::None:
        count(line, 0);
        if (info->mnemonic == "RET") word = encodeI(op, 0, 13, 0);
        else if (info->opcode == Opcode::HALT) word = encodeJ(op, 0);
        else word = encodeR(op, 0, 0, 0, info->funct);
        break;
    case OperandForm::RRR:
        count(line, 3); word = encodeR(op, reg(line.operands[0]), reg(line.operands[1]), reg(line.operands[2]), info->funct); break;
    case OperandForm::RRI:
        count(line, 3); word = encodeI(op, reg(line.operands[0]), reg(line.operands[1]), signedImm(line.operands[2], 20)); break;
    case OperandForm::RI:
        count(line, 2);
        if (info->mnemonic == "LUI") {
            const auto imm = number(line.operands[1]);
            if (!fitsUnsigned(imm, 20)) error(line.operands[1], "immediate does not fit unsigned 20-bit range");
            word = encodeIU(op, reg(line.operands[0]), 0, static_cast<std::uint32_t>(imm));
        } else word = encodeI(op, reg(line.operands[0]), 0, signedImm(line.operands[1], 20));
        break;
    case OperandForm::RRTarget: {
        count(line, 3); const auto offset = target(line.operands[2], symbols, pc);
        if (offset % 4 != 0) error(line.operands[2], "branch offset must be divisible by 4");
        if (!fitsSigned(offset, 20)) error(line.operands[2], "branch offset does not fit signed 20-bit range");
        word = encodeI(op, reg(line.operands[0]), reg(line.operands[1]), static_cast<std::int32_t>(offset)); break;
    }
    case OperandForm::Target: {
        count(line, 1); const auto offset = target(line.operands[0], symbols, pc);
        if (offset % 4 != 0) error(line.operands[0], "jump offset must be divisible by 4");
        if (!fitsSigned(offset, 28)) error(line.operands[0], "jump offset does not fit signed 28-bit range");
        word = encodeJ(op, static_cast<std::int32_t>(offset)); break;
    }
    }
    if (!isLegalInstruction(word)) error(line.mnemonic, "assembler produced an illegal instruction");
    return word;
}
} // namespace

AssemblyResult assemble(std::string_view source) {
    const auto lines = parse(source);
    SymbolTable symbols;
    std::uint64_t address = 0;
    for (const auto& line : lines) {
        if (!line.label.empty()) symbols.define(line.label, static_cast<std::uint32_t>(address), line.labelLocation);
        if (line.hasInstruction) address += 4;
        if (address > std::numeric_limits<std::uint32_t>::max()) throw SourceError(line.mnemonic.location, "program is too large");
    }
    AssemblyResult result;
    std::uint32_t pc = 0;
    for (const auto& line : lines) if (line.hasInstruction) {
        const auto word = encode(line, symbols, pc);
        result.words.push_back(word);
        for (unsigned shift = 0; shift < 32; shift += 8) result.bytes.push_back(static_cast<std::uint8_t>(word >> shift));
        pc += 4;
    }
    return result;
}
} // namespace helium::assembler
