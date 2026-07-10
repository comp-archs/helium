#include "parser.hpp"

#include <cctype>
#include <sstream>
#include <utility>

namespace helium::assembler {
namespace {
bool space(char c) { return c == ' ' || c == '\t' || c == '\r'; }
std::size_t skipSpace(std::string_view s, std::size_t p) { while (p < s.size() && space(s[p])) ++p; return p; }
[[noreturn]] void fail(std::size_t line, std::size_t col, const std::string& message) {
    throw SourceError({line, col}, message);
}
}

SourceError::SourceError(SourceLocation location, const std::string& message)
    : std::runtime_error(message), location_(location) {}

bool isValidIdentifier(std::string_view text) {
    if (text.empty() || !(std::isalpha(static_cast<unsigned char>(text[0])) || text[0] == '_')) return false;
    for (char c : text.substr(1))
        if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_')) return false;
    return true;
}

std::vector<ParsedLine> parse(std::string_view source) {
    std::vector<ParsedLine> result;
    std::istringstream input{std::string(source)};
    std::string owned;
    std::size_t lineNo = 0;
    while (std::getline(input, owned)) {
        ++lineNo;
        std::string_view line(owned);
        if (const auto comment = line.find(';'); comment != std::string_view::npos) line = line.substr(0, comment);
        ParsedLine parsed;
        std::size_t p = skipSpace(line, 0);
        if (p == line.size()) continue;

        const auto colon = line.find(':', p);
        const auto firstSpace = line.find_first_of(" \t\r", p);
        if (colon != std::string_view::npos && (firstSpace == std::string_view::npos || colon < firstSpace)) {
            const auto name = line.substr(p, colon - p);
            if (!isValidIdentifier(name)) fail(lineNo, p + 1, "invalid label");
            parsed.label = std::string(name);
            parsed.labelLocation = {lineNo, p + 1};
            p = skipSpace(line, colon + 1);
            if (p == line.size()) { result.push_back(std::move(parsed)); continue; }
        } else if (colon != std::string_view::npos) {
            fail(lineNo, colon + 1, "unexpected ':'");
        }

        const std::size_t mnemonicStart = p;
        while (p < line.size() && !space(line[p]) && line[p] != ',') ++p;
        if (p == mnemonicStart) fail(lineNo, p + 1, "expected mnemonic");
        parsed.mnemonic = {std::string(line.substr(mnemonicStart, p - mnemonicStart)), {lineNo, mnemonicStart + 1}};
        parsed.hasInstruction = true;
        p = skipSpace(line, p);
        if (p < line.size() && line[p] == ',') fail(lineNo, p + 1, "unexpected comma");
        while (p < line.size()) {
            const std::size_t start = p;
            while (p < line.size() && line[p] != ',') ++p;
            std::size_t end = p;
            while (end > start && space(line[end - 1])) --end;
            if (end == start) fail(lineNo, start + 1, "missing operand");
            parsed.operands.push_back({std::string(line.substr(start, end - start)), {lineNo, start + 1}});
            if (p == line.size()) break;
            const std::size_t commaColumn = p + 1;
            ++p;
            p = skipSpace(line, p);
            if (p == line.size()) fail(lineNo, commaColumn, "missing operand after comma");
        }
        result.push_back(std::move(parsed));
    }
    return result;
}
} // namespace helium::assembler
