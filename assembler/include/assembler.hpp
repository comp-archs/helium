#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace helium::assembler {

struct AssemblyResult {
    std::vector<std::uint32_t> words;
    std::vector<std::uint8_t> bytes;
};

AssemblyResult assemble(std::string_view source);

} // namespace helium::assembler
