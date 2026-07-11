#### Helium ISA Specification
#### 1. Scope

Helium is a small RISC-inspired 32-bit instruction-set architecture. Version
0.1 defines the architectural state, instruction encodings and behavior needed
to assemble and execute small freestanding programs and functions.

Cycle timing, pipeline organization, caches, interrupts, virtual memory and a
host I/O ABI are outside the v0.1 ISA. They may be specified by later
microarchitecture and execution-environment documents without changing these
instruction semantics.

| Property | Value |
|---|---|
| Word and address size | 32 bits |
| Byte order | Little-endian |
| Address space | 32-bit, byte-addressable |
| Instruction width | Fixed 32 bits |
| Instruction alignment | 4 bytes |
| Register file | 16 general-purpose registers, R0-R15 |
| Program counter | Separate 32-bit PC |

All registers, addresses and arithmetic results are 32-bit bit-vectors unless
an instruction explicitly specifies a signed interpretation.

#### 2. Architectural state

| Register | Alias | Role |
|---|---|---|
| R0 | ZERO | Hard-wired zero; reads return 0 and writes are discarded |
| R1-R12 | - | General purpose |
| R13 | LR | Link register by convention |
| R14 | FP | Frame pointer by convention |
| R15 | SP | Stack pointer by convention |

The PC is not part of the general-purpose register file. An implementation also
has one of three execution states:

- `Running`
- `Halted`
- `Faulted`, with the fault information defined in section 8

Source operands are read from the pre-instruction state. A destination of R0 is
legal; the operation still executes and can fault, but its register result is
discarded.

#### 3. Instruction encodings

All fields are shown most-significant bit first.

#### 3.1 R-type

```text
 31      28 27   24 23   20 19   16 15         4 3       0
 +---------+-------+-------+-------+-------------+---------+
 | opcode  |  rd   |  rs1  |  rs2 |  reserved   |  funct  |
 +---------+-------+-------+-------+-------------+---------+
```

Bits 15-4 must be zero. Opcode `0x0` selects the ALU class and `funct`
selects the operation.

#### 3.2 I-type

```text
 31      28 27   24 23   20 19                          0
 +---------+-------+-------+-------------------------------+
 | opcode  |  rd   |  rs1  |             imm20             |
 +---------+-------+-------+-------------------------------+
```

Except for `LUI`, `imm20` is a signed two's-complement value in the range
`-524288` through `524287`. `LUI` treats the field as an unsigned 20-bit value.

The `rd` field is the value to store for `ST` and the first comparison operand
for `BEQ` and `BNE`; those instructions do not write a destination register.

#### 3.3 J-type

```text
 31      28 27                                           0
 +---------+----------------------------------------------+
 | opcode  |          signed PC-relative offset28         |
 +---------+----------------------------------------------+
```

The offset is a signed two's-complement byte offset in the range `-134217728`
through `134217727`.

`HALT` uses the J-type physical layout but has no assembly operands and requires
all 28 payload bits to be zero.

#### 4. Final opcode allocation

#### 4.1 Primary opcodes

| Opcode | Mnemonic/class | Encoding | Assembly operands |
|---:|---|---|---|
| `0x0` | ALU class | R | Selected by `funct` |
| `0x1` | Reserved | - | - |
| `0x2` | `ADDI` | I | `rd, rs1, #imm20` |
| `0x3` | `LUI` | I | `rd, #uimm20` |
| `0x4` | `LD` | I | `rd, base, #imm20` |
| `0x5` | `ST` | I | `src, base, #imm20` |
| `0x6` | `BEQ` | I | `ra, rb, target` |
| `0x7` | `BNE` | I | `ra, rb, target` |
| `0x8` | `JMP` | J | `target` |
| `0x9` | `JAL` | J | `target` |
| `0xA` | `JALR` | I | `rd, base, #imm20` |
| `0xB` | `HALT` | J payload fixed to zero | none |
| `0xC-0xF` | Reserved | - | - |

#### 4.2 ALU functions for opcode `0x0`

| Funct | Mnemonic | Result |
|---:|---|---|
| `0x0` | `NOP` | No architectural effect |
| `0x1` | `ADD` | `rd = rs1 + rs2` |
| `0x2` | `SUB` | `rd = rs1 - rs2` |
| `0x3` | `AND` | `rd = rs1 & rs2` |
| `0x4` | `OR` | `rd = rs1 \| rs2` |
| `0x5` | `XOR` | `rd = rs1 ^ rs2` |
| `0x6` | `SHL` | `rd = rs1 << (rs2 & 31)` |
| `0x7` | `SHR` | Logical `rd = rs1 >> (rs2 & 31)` |
| `0x8` | `SAR` | Arithmetic `rd = signed(rs1) >> (rs2 & 31)` |
| `0x9` | `SLT` | `rd = signed(rs1) < signed(rs2) ? 1 : 0` |
| `0xA` | `SLTU` | `rd = unsigned(rs1) < unsigned(rs2) ? 1 : 0` |
| `0xB-0xF` | Reserved | Illegal instruction |

`ADD`, `SUB`, `ADDI`, effective-address calculations and PC calculations wrap
modulo 2^32. They do not set flags or raise arithmetic-overflow faults.

#### 5. Instruction semantics

Let `P` be the address of the executing instruction. Unless control flow is
taken or a fault occurs, the next PC is `P + 4`, modulo 2^32.

#### 5.1 Immediate and upper-immediate operations

- `ADDI rd, rs1, #imm20`: `R[rd] = R[rs1] + sign_extend(imm20)`.
- `LUI rd, #uimm20`: `R[rd] = zero_extend(uimm20) << 12`. The encoded `rs1`
  field must be zero.

#### 5.2 Loads and stores

The effective address is `R[base] + sign_extend(imm20)`, modulo 2^32.

- `LD` reads four consecutive bytes in little-endian order.
- `ST` writes the low-to-high bytes of `R[src]` in little-endian order.
- The effective address must be divisible by four.
- All four bytes must be present in readable or writable mapped memory.
- `LD R0, ...` still performs the access and can fault.
- `ST R0, ...` stores four zero bytes.

#### 5.3 Branches and direct jumps

Branch and jump immediates are signed **byte offsets relative to `P`**, not to
`P + 4`:

- `BEQ ra, rb, target` transfers to `P + sign_extend(offset20)` when the full
  32-bit values are equal.
- `BNE ra, rb, target` transfers when the values are unequal.
- `JMP target` always transfers to `P + sign_extend(offset28)`.
- `JAL target` first computes the target, then writes `P + 4` to LR (R13) and
  transfers to the target.

For a label, the assembler computes `label_address - instruction_address` in a
wider signed host type. It rejects an offset that is not divisible by four or
does not fit its field. Consequently, the aligned I-type control range is
`-524288` through `524284`, and the aligned J-type range is `-134217728`
through `134217724`.

A conditional branch checks target alignment only when it is taken. Direct
jumps always check target alignment. An aligned target is not checked for
executable memory until the subsequent instruction fetch. Thus, a `JAL` or
`JALR` to an aligned but unmapped address commits its link and target PC before
that next fetch raises `instruction-access-fault`.

#### 5.4 Indirect jumps

`JALR rd, base, #imm20` behaves as follows:

1. Read `R[base]` from the pre-instruction state.
2. Compute `target = R[base] + sign_extend(imm20)` modulo 2^32.
3. Require `target` to be divisible by four.
4. Write `P + 4` to `rd` and set PC to `target`.

The implementation does not clear or otherwise modify low target bits. If
`rd == base`, the old base value is used. If `rd == R0`, the link is discarded.
A target fault occurs before the link is written.

#### 5.5 Halt

The only legal `HALT` encoding is `0xB0000000`. It retires, advances PC to
`P + 4`, and enters `Halted`. No instruction is fetched while halted. Execution
can resume only after reset.

Falling past mapped memory, executing zero-filled memory as repeated NOPs, or
encountering an illegal instruction is not an implicit successful halt.

#### 6. Canonical and illegal encodings

Decoders must reject the following as illegal instructions:

- Primary opcodes `0x1` and `0xC-0xF`.
- ALU function values `0xB-0xF`.
- An opcode-`0x0` word with nonzero bits 15-4.
- A `NOP` with any nonzero `rd`, `rs1` or `rs2` field. Its only legal encoding
  is `0x00000000`.
- A `LUI` with a nonzero encoded `rs1` field.
- A `HALT` with any nonzero payload bit.

Register fields that name R0 are otherwise legal. A misaligned encoded control
offset is not itself an illegal instruction: it raises an instruction-address
fault if the transfer is performed.

#### 7. Reset and execution environment

Architectural reset performs the following actions:

- Sets PC to `0x00000000`.
- Sets R1-R15 to zero; R0 remains hard-wired to zero.
- Sets execution state to `Running`.
- Does not initialize, clear or rewrite memory.

The initial v0.1 emulator execution profile will provide zero-initialized RAM,
load a raw binary at address zero, reset the core and require software to
initialize SP. The amount of mapped RAM and host I/O mechanism are execution
environment choices rather than ISA properties.

#### 8. Fault model

Version 0.1 has no trap vector or exception registers. A fault enters the
terminal `Faulted` state and records at least:

- Fault cause.
- Faulting PC.
- Fetched instruction word, when available.
- Bad memory or control address, when applicable.

Defined causes are:

- `instruction-address-misaligned`
- `instruction-access-fault`
- `illegal-instruction`
- `load-address-misaligned`
- `load-access-fault`
- `store-address-misaligned`
- `store-access-fault`

Faults are precise: except for entering `Faulted` and recording fault
information, no effects of the faulting instruction are committed. PC remains
at `P`; no register, link or memory value is modified. In particular, a
faulting store performs no partial write and a `JAL` or `JALR` that faults on
target alignment writes no link.

Fault priority is:

1. Instruction PC alignment.
2. Instruction fetch/access.
3. Illegal instruction encoding.
4. Instruction-specific execution checks.

For loads and stores, address alignment takes priority over memory mapping or
access checks.

#### 9. Assembly language baseline

Assembly is case-insensitive for mnemonics and register names. `;` starts a
comment. `#` prefixes a numeric immediate; labels are written without `#`.

```asm
start:
    LDI   R1, #10          ; pseudo: ADDI R1, R0, #10
    ADDI  SP, SP, #-4
    ST    LR, SP, #0
loop:
    ADDI  R1, R1, #-1
    BNE   R1, R0, loop
    LD    LR, SP, #0
    ADDI  SP, SP, #4
    RET                    ; pseudo: JALR R0, LR, #0
```

The initial assembler defines these pseudos:

| Pseudo | Expansion |
|---|---|
| `LDI rd, #imm20` | `ADDI rd, R0, #imm20` |
| `RET` | `JALR R0, LR, #0` |

`LUI` and `ADDI` are sufficient to construct arbitrary 32-bit constants. A
future `LI` pseudo may select one or two instructions without changing the ISA.

#### 10. Calling convention v0.1

- Arguments: R1-R4.
- Return value: R1.
- Caller-saved: R1-R7 and LR.
- Callee-saved: R8-R12, FP and SP.
- `JAL` writes its return address to LR.
- `RET` returns through LR.
- The stack grows toward lower addresses and SP remains four-byte aligned.

#### 11. Canonical golden vectors

These words are normative encoding vectors. Binary output writes each word in
little-endian byte order.

| Assembly | Word | Little-endian bytes |
|---|---:|---|
| `NOP` | `0x00000000` | `00 00 00 00` |
| `ADD R1, R2, R3` | `0x01230001` | `01 00 23 01` |
| `SUB R1, R2, R3` | `0x01230002` | `02 00 23 01` |
| `AND R1, R2, R3` | `0x01230003` | `03 00 23 01` |
| `OR R1, R2, R3` | `0x01230004` | `04 00 23 01` |
| `XOR R1, R2, R3` | `0x01230005` | `05 00 23 01` |
| `SHL R1, R2, R3` | `0x01230006` | `06 00 23 01` |
| `SHR R1, R2, R3` | `0x01230007` | `07 00 23 01` |
| `SAR R1, R2, R3` | `0x01230008` | `08 00 23 01` |
| `SLT R1, R2, R3` | `0x01230009` | `09 00 23 01` |
| `SLTU R1, R2, R3` | `0x0123000A` | `0A 00 23 01` |
| `ADDI R1, R2, #-1` | `0x212FFFFF` | `FF FF 2F 21` |
| `LUI R1, #0xABCDE` | `0x310ABCDE` | `DE BC 0A 31` |
| `LD R1, R2, #16` | `0x41200010` | `10 00 20 41` |
| `ST R1, R2, #-16` | `0x512FFFF0` | `F0 FF 2F 51` |
| `BEQ R1, R2, #16` | `0x61200010` | `10 00 20 61` |
| `BNE R1, R2, #-4` | `0x712FFFFC` | `FC FF 2F 71` |
| `JMP #16` | `0x80000010` | `10 00 00 80` |
| `JAL #-4` | `0x9FFFFFFC` | `FC FF FF 9F` |
| `JALR R1, R2, #-4` | `0xA12FFFFC` | `FC FF 2F A1` |
| `HALT` | `0xB0000000` | `00 00 00 B0` |
| `LDI R1, #-1` | `0x210FFFFF` | `FF FF 0F 21` |
| `RET` | `0xA0D00000` | `00 00 D0 A0` |
