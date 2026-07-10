#include "symbols.hpp"
#include <iostream>

using namespace helium::assembler;
int main() {
    int failures = 0; SymbolTable symbols; symbols.define("Foo", 12, {1, 1}); symbols.define("foo", 16, {2, 1});
    if (!symbols.contains("Foo") || symbols.resolve("Foo", {1, 1}) != 12 || symbols.resolve("foo", {1, 1}) != 16) ++failures;
    try { symbols.define("Foo", 20, {3, 1}); ++failures; } catch (const SourceError&) {}
    try { (void)symbols.resolve("missing", {4, 2}); ++failures; } catch (const SourceError& e) { if (e.location().line != 4) ++failures; }
    if (failures) { std::cerr << "FAIL test_symbols: " << failures << '\n'; return 1; }
    std::cout << "PASS test_symbols\n"; return 0;
}
