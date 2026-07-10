#include "parser.hpp"
#include <iostream>

using namespace helium::assembler;
int main() {
    int failures = 0;
    const auto lines = parse(" ; blank\n  Start: add R1, r2, R3 ; comment\nlabel:\n");
    if (lines.size() != 2 || lines[0].label != "Start" || lines[0].mnemonic.text != "add" ||
        lines[0].mnemonic.location.line != 2 || lines[0].mnemonic.location.column != 10 ||
        lines[0].operands.size() != 3 || lines[0].operands[1].location.column != 18 || lines[1].hasInstruction) ++failures;
    for (const char* bad : {"1bad: NOP\n", "NOP , R1\n", "ADD R1,,R2\n", "ADD R1,\n", "foo bar: NOP\n"}) {
        try { (void)parse(bad); ++failures; } catch (const SourceError&) {}
    }
    for (const char* trailingComma : {"ADD R1,\n", "ADD R1,   \n", "ADD R1,\r\n"}) {
        try {
            (void)parse(trailingComma);
            ++failures;
        } catch (const SourceError& error) {
            if (error.location().line != 1 || error.location().column != 7) ++failures;
        }
    }
    if (!isValidIdentifier("_x9") || isValidIdentifier("9x")) ++failures;
    if (failures) { std::cerr << "FAIL test_parser: " << failures << '\n'; return 1; }
    std::cout << "PASS test_parser\n"; return 0;
}
