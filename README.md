# Helium

Helium is a **RISC-inspired 32-bit instruction set architecture (ISA)** designed from scratch, along with tooling to make it practical to experiment with and implement.

## What this project achieves

- Defines a clean, fixed-width **32-bit ISA** aimed at simplicity and ease of implementation.
- Provides (or intends to provide) a full toolchain around the ISA:
  - **Assembler** for Helium assembly
  - **Cycle-accurate emulator** for executing Helium programs
  - **Interactive debugger** for stepping through execution and inspecting state

## ISA highlights (v0.1)

- 32-bit word size, **little-endian**
- **16 general-purpose registers** (R0–R15)
  - R0 is hard-wired to zero
  - PC is separate; R15 is stack pointer by convention
- **Fixed 32-bit instruction width**
- Three encoding formats: **R-type**, **I-type**, **J-type**
- ALU operations share one primary opcode and use an R-type function field
- Instructions include `ADD`, `SUB`, `AND`, `OR`, `XOR`, shifts, comparisons,
  `ADDI`, `LUI`, `LD`, `ST`, branches, direct and indirect jumps, and `HALT`
- `LDI` and `RET` are assembler pseudo-instructions

See the full spec: [`spec/isa.md`](spec/isa.md)

## Repository layout

- `spec/` — ISA specification and design notes
- `assembler/` — two-pass assembler and command-line tool
- `emulator/` — emulator/debugger implementation (WIP)
- `tests/` — cross-component test programs and validation (planned)

## Status

The Helium ISA v0.1 encoding and architectural semantics are frozen. The Phase
1 assembler MVP is complete; the emulator and debugger remain works in
progress.
