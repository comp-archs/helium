#pragma once

#include "parser.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

namespace helium::assembler {

class SymbolTable {
public:
    void define(std::string name, std::uint32_t address, SourceLocation location);
    std::uint32_t resolve(std::string_view name, SourceLocation location) const;
    bool contains(std::string_view name) const;
private:
    std::unordered_map<std::string, std::uint32_t> symbols_;
};

} // namespace helium::assembler
