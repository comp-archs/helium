# Helium ISA Specification

> **Status:** Work in progress — updated as the ISA is designed.

---

## 1. Overview

Helium is a RISC-inspired 32-bit instruction-set architecture designed from
scratch.  The goals are simplicity, orthogonality, and ease of implementation
in a cycle-accurate software emulator.

| Property | Value |
|---|---|
| Word size | 32 bits |
| Endianness | Little-endian |
| Register file | 16 general-purpose registers (R0 – R15) |
| Program counter | PC (separate, not part of the GPR file) |
| Stack pointer | R15 (by convention) |
| Instruction width | Fixed 32-bit |

---

## 2. Register File

| Register | Alias | Role |
|---|---|---|
| R0 | zero | Hard-wired to 0 (reads always return 0; writes are discarded) |
| R1 – R12 | — | General purpose |
| R13 | LR | Link register (return address) |
| R14 | FP | Frame pointer (by convention) |
| R15 | SP | Stack pointer (by convention) |

---

## 3. Instruction Formats

All instructions are 32 bits wide.  Three primary encoding formats are defined:

### 3.1 R-Type (Register–Register)

```
 31      28 27   24 23   20 19   16 15         4 3       0
 +---------+-------+-------+-------+------------+---------+
 |  opcode |  rd   |  rs1  |  rs2  |  (unused)  |  funct  |
 +---------+-------+-------+-------+------------+---------+
```

### 3.2 I-Type (Immediate)

```
 31      28 27   24 23   20 19                          0
 +---------+-------+-------+-------------------------------+
 |  opcode |  rd   |  rs1  |         imm16 (sign-ext)      |
 +---------+-------+-------+-------------------------------+
```

### 3.3 J-Type (Jump)

```
 31      28 27                                           0
 +---------+----------------------------------------------+
 |  opcode |               target (sign-ext, PC-relative) |
 +---------+----------------------------------------------+
```

---

## 4. Opcode Table

> Opcodes are 4 bits (values 0x0 – 0xF).  The table will be filled in as
> instructions are finalised.

| Opcode | Mnemonic | Format | Description |
|---|---|---|---|
| 0x0 | NOP | R | No operation |
| 0x1 | ADD | R | `rd = rs1 + rs2` |
| 0x2 | SUB | R | `rd = rs1 - rs2` |
| 0x3 | AND | R | `rd = rs1 & rs2` |
| 0x4 | OR  | R | `rd = rs1 \| rs2` |
| 0x5 | XOR | R | `rd = rs1 ^ rs2` |
| 0x6 | SHL | R | `rd = rs1 << rs2[3:0]` |
| 0x7 | SHR | R | `rd = rs1 >> rs2[3:0]` (logical) |
| 0x8 | LDI | I | `rd = sign_ext(imm16)` |
| 0x9 | LD  | I | `rd = mem[rs1 + sign_ext(imm16)]` |
| 0xA | ST  | I | `mem[rs1 + sign_ext(imm16)] = rd` |
| 0xB | BEQ | I | `if rd == rs1: PC += sign_ext(imm16)` |
| 0xC | BNE | I | `if rd != rs1: PC += sign_ext(imm16)` |
| 0xD | JMP | J | `PC += sign_ext(target)` |
| 0xE | JAL | J | `LR = PC + 4; PC += sign_ext(target)` |
| 0xF | *(reserved)* | — | — |

---

## 5. Memory Model

- Address space: 32-bit (4 GiB), byte-addressable.
- Load/store granularity: 32-bit words (aligned).
- Unaligned accesses raise an alignment fault.

---

## 6. Calling Convention (draft)

- Arguments: R1 – R4.
- Return values: R1.
- Callee-saved: R8 – R12, FP, SP.
- Caller-saved: R1 – R7, LR.

---

## 7. Assembly Syntax

```asm
; This is a comment
label:
    MNEMONIC  rd, rs1, rs2     ; R-type
    MNEMONIC  rd, rs1, #imm    ; I-type
    MNEMONIC  #offset          ; J-type
```

---

*This document will be expanded as the assembler and emulator are built.*
