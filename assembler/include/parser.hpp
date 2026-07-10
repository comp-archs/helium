#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace helium::assembler {

struct SourceLocation { std::size_t line = 1; std::size_t column = 1; };
struct Token { std::string text; SourceLocation location; };
struct ParsedLine {
    std::string label;
    SourceLocation labelLocation{};
    Token mnemonic;
    std::vector<Token> operands;
    bool hasInstruction = false;
};

class SourceError : public std::runtime_error {
public:
    SourceError(SourceLocation location, const std::string& message);
    SourceLocation location() const noexcept { return location_; }
private:
    SourceLocation location_;
};

// Labels are case-sensitive and match [A-Za-z_][A-Za-z0-9_]*.
bool isValidIdentifier(std::string_view text);
std::vector<ParsedLine> parse(std::string_view source);

} // namespace helium::assembler
