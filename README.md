#### Helium

Helium is a **RISC-inspired 32-bit instruction set architecture (ISA)** designed from scratch, along with tooling to make it practical to experiment with and implement.

#### What this project achieves

- Defines a clean, fixed-width **32-bit ISA** aimed at simplicity and ease of implementation.
- Provides (or intends to provide) a full toolchain around the ISA:
  - **Assembler** for Helium assembly
  - **Cycle-accurate emulator** for executing Helium programs
  - **Interactive debugger** for stepping through execution and inspecting state

#### ISA highlights (current spec)

- 32-bit word size, **little-endian**
- **16 general-purpose registers** (R0–R15)
  - R0 is hard-wired to zero
  - PC is separate; R15 is stack pointer by convention
- **Fixed 32-bit instruction width**
- Three encoding formats: **R-type**, **I-type**, **J-type**
- Draft opcode set includes: `ADD`, `SUB`, ``AND`, `OR`, `XOR`, `SHL`, `SHR`, `LDI`, `LD`, `ST`, `BEQ`, `BNE`, `JMP`, `JAL`

See the full spec: [`spec/isa.md`](spec/isa.md)

#### Repository layout

- `spec/` — ISA specification and design notes
- `assembler/` — assembler implementation (WIP)
- `emulator/` — emulator/debugger implementation (WIP)
- `tests/` — test programs and validation (WIP)

#### Status

Work in progress: the ISA spec is being updated as the design is finalized, and tooling is being built alongside it.
