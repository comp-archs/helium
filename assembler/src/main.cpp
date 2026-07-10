#include "assembler.hpp"
#include "parser.hpp"

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {

void usage(std::ostream& out) {
    out << "Usage: helium-asm <input.asm> -o <output.bin>\n";
}

} // namespace

int main(int argc, char** argv) {
    std::string inputPath;
    std::string outputPath;

    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--help") {
            usage(std::cout);
            return 0;
        }
        if (arg == "-o" || arg == "--output") {
            if (++i >= argc || !outputPath.empty()) {
                std::cerr << "error: missing or repeated output path\n";
                usage(std::cerr);
                return 2;
            }
            outputPath = argv[i];
        } else if (!arg.empty() && arg[0] == '-') {
            std::cerr << "error: unknown option '" << arg << "'\n";
            return 2;
        } else if (!inputPath.empty()) {
            std::cerr << "error: exactly one input file is required\n";
            return 2;
        } else {
            inputPath = arg;
        }
    }

    if (inputPath.empty() || outputPath.empty()) {
        std::cerr << "error: input and -o/--output are required\n";
        usage(std::cerr);
        return 2;
    }

    std::ifstream input(inputPath, std::ios::binary);
    if (!input) {
        std::cerr << inputPath << ": error: cannot open input\n";
        return 1;
    }
    const std::string source((std::istreambuf_iterator<char>(input)),
                             std::istreambuf_iterator<char>());

    try {
        const auto result = helium::assembler::assemble(source);
        std::ofstream output(outputPath, std::ios::binary | std::ios::trunc);
        if (!output) {
            std::cerr << outputPath << ": error: cannot open output\n";
            return 1;
        }
        output.write(reinterpret_cast<const char*>(result.bytes.data()),
                     static_cast<std::streamsize>(result.bytes.size()));
        output.close();
        if (!output) {
            std::cerr << outputPath << ": error: failed writing output\n";
            return 1;
        }
    } catch (const helium::assembler::SourceError& error) {
        std::cerr << inputPath << ':' << error.location().line << ':' << error.location().column
                  << ": error: " << error.what() << '\n';
        return 1;
    } catch (const std::exception& error) {
        std::cerr << inputPath << ": error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
