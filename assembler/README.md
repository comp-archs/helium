#### Helium Assembler (assembler)

A compact assembler component for the Helium ISA.

#### What this component does

- Parses Helium assembly instructions.
- Validates mnemonics/registers against ISA definitions.
- Encodes instructions into 32-bit machine words.
- (Planned) resolves labels/symbols and emits output binaries.

#### Current structure

- `include/isa.hpp` / `src/isa.cpp`  
  ISA metadata and lookup helpers:
  - mnemonics ↔ opcode/format
  - register parsing (e.g., `R0..R15`, aliases)

- `include/encoding.hpp` / `src/encoding.cpp`  
  Bit-packing and extraction:
  - `encodeR`, `encodeI`, `encodeJ`
  - range checks and encoding exceptions
  - field extractors for tests/debug

- `tests/test_encoding.cpp`  
  Unit tests for encoding correctness and boundary behavior.

#### Build

```bash
cmake -S assembler -B build -G Ninja
cmake --build build
```

#### Run tests

If tests are wired in CMake:

```bash
ctest --test-dir build --output-on-failure
```

Or run test binary directly (example):

```bash
./build/helium_asm_tests
```

#### Notes

- Project uses C++20.
- If your editor shows false errors for `<optional>`/`<string_view>`, ensure it reads `build/compile_commands.json`.
