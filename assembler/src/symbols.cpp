#include "symbols.hpp"

#include <utility>

namespace helium::assembler {
void SymbolTable::define(std::string name, std::uint32_t address, SourceLocation location) {
    if (!symbols_.emplace(std::move(name), address).second) throw SourceError(location, "duplicate label");
}
std::uint32_t SymbolTable::resolve(std::string_view name, SourceLocation location) const {
    const auto it = symbols_.find(std::string(name));
    if (it == symbols_.end()) throw SourceError(location, "undefined label '" + std::string(name) + "'");
    return it->second;
}
bool SymbolTable::contains(std::string_view name) const { return symbols_.contains(std::string(name)); }
} // namespace helium::assembler
