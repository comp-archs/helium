#### Helium Assembler wip

A compact assembler component for the Helium ISA.

#### Implemented capabilities

- Parses and validates every physical ISA v0.1 instruction plus `LDI` and
  `RET`.
- Resolves forward and backward labels in two passes and emits raw
  little-endian bytes.
- Reports source errors with one-based line and column locations.

#### Syntax

Source is line-oriented. `;` begins a comment, operands are comma-separated,
and a label may appear alone or before an instruction. Labels are case-sensitive
and match `[A-Za-z_][A-Za-z0-9_]*`; mnemonics and registers are
case-insensitive. Numeric immediates require `#` and accept signed decimal or
hexadecimal forms, such as `#16`, `#-4`, and `#-0x10`.

Control-flow labels are converted to byte offsets relative to the current
instruction. Numeric control operands are already byte offsets. Direct jump and
branch offsets must be divisible by four; `JALR` immediates need not be.

```asm
start: LDI R1, #10
loop:  ADDI R1, R1, #-1
       BNE R1, ZERO, loop
       RET
```

```bash
./build/test_assembler
```

- Project uses C++20.
- If your editor shows false errors for `<optional>`/`<string_view>`, ensure it reads `build/compile_commands.json`.
