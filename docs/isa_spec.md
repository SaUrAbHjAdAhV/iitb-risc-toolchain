# IITB-RISC-23 — Instruction Set Architecture Specification

## Overview
- 16-bit computer, 8 general-purpose registers (R0–R7)
- R0 is the Program Counter (PC) — not a general-purpose register. Every instruction
  implicitly advances R0 by 2 (byte-addressed, 2-byte instructions) unless overwritten
  by a branch/jump target.
- Byte-addressed memory; every instruction/data fetch is 2 bytes.
- Condition code register: Carry (C) flag, Zero (Z) flag.
- Supports predicated execution (R-type ADD/NAND families gated on C or Z) and
  multi-register load/store (LM/SM).
- 3 instruction formats: R, I, J.
- 26 distinct mnemonics.

## Register reference
| Register | Binary | Purpose |
|---|---|---|
| R0 | 000 | Program Counter — special, not general purpose |
| R1–R7 | 001–111 | General purpose |

## Instruction formats (bit 15 = MSB)

**R-type** — used by ADD-family and NAND-family
| Bits | 15–12 | 11–9 | 8–6 | 5–3 | 2 | 1–0 |
|---|---|---|---|---|---|---|
| Field | opcode | RA | RB | RC | Complement | CZ |

**I-type**
| Bits | 15–12 | 11–9 | 8–6 | 5–0 |
|---|---|---|---|---|
| Field | opcode | RA | RB | Imm6 (signed) |

⚠ Register roles are NOT uniform across I-type instructions — see per-instruction
semantics below, not a fixed template.

## I-type operand-to-field mapping (read carefully — order is not uniform)

| Mnemonic | Assembly order | 1st operand → | 2nd operand → |
|---|---|---|---|
| ADI | `adi rb, ra, imm6` | RB bit-slot | RA bit-slot |
| LW  | `lw ra, rb, imm6`  | RA bit-slot | RB bit-slot |
| SW  | `sw ra, rb, imm6`  | RA bit-slot | RB bit-slot |
| BEQ | `beq ra, rb, imm6` | RA bit-slot | RB bit-slot |
| BLT | `blt ra, rb, imm6` | RA bit-slot | RB bit-slot |
| BLE | `ble ra, rb, imm6` | RA bit-slot | RB bit-slot |
| JLR | `jlr ra, rb`       | RA bit-slot | RB bit-slot |

Only ADI has swapped order. Everything else matches assembly order to bit order —
but write the per-instruction mapping explicitly in code anyway (e.g. a small table
indexed by opcode), don't rely on "everything except ADI" as a mental rule you'll
still be holding correctly on day 12.


**J-type**
| Bits | 15–12 | 11–9 | 8–0 |
|---|---|---|---|
| Field | opcode | RA | Imm9 |

⚠ Imm9 is sign-extended for JAL/JRI (branch offsets), but zero-extended for LLI
(loading a literal). Treat per-instruction, not as a blanket format rule.

## CZ field semantics (R-type ADD/NAND families)
| CZ | Meaning |
|---|---|
| 00 | Unconditional |
| 01 | Execute only if Z flag set |
| 10 | Execute only if C flag set |
| 11 | ADD-family only: use Carry as a third operand (not a gate). Unused/reserved for NAND-family. |

Complement bit: when 1, RB is bitwise-inverted before the operation
(`ACA` etc. = `RA + ~RB`; `ACW` = `RA + ~RB + Carry`, the classic two's-complement subtract trick).

## Instruction table

### ADD family — opcode `0001` (R-type)
| Mnemonic | Comp | CZ | Semantics |
|---|---|---|---|
| ADA | 0 | 00 | `RC = RA + RB` (unconditional). Updates C, Z. |
| ADZ | 0 | 01 | `RC = RA + RB` if Z set. Updates C, Z. |
| ADC | 0 | 10 | `RC = RA + RB` if C set. Updates C, Z. |
| AWC | 0 | 11 | `RC = RA + RB + Carry` (unconditional). Updates C, Z. |
| ACA | 1 | 00 | `RC = RA + ~RB` (unconditional). Updates C, Z. |
| ACZ | 1 | 01 | `RC = RA + ~RB` if Z set. Updates C, Z. |
| ACC | 1 | 10 | `RC = RA + ~RB` if C set. Updates C, Z. |
| ACW | 1 | 11 | `RC = RA + ~RB + Carry` (unconditional). Updates C, Z. |

### NAND family — opcode `0010` (R-type)
| Mnemonic | Comp | CZ | Semantics |
|---|---|---|---|
| NDU | 0 | 00 | `RC = NAND(RA, RB)` (unconditional). Updates Z. |
| NDZ | 0 | 01 | `RC = NAND(RA, RB)` if Z set. Updates Z. |
| NDC | 0 | 10 | `RC = NAND(RA, RB)` if C set. Updates Z. |
| NCU | 1 | 00 | `RC = NAND(RA, ~RB)` (unconditional). Updates Z. |
| NCZ | 1 | 01 | `RC = NAND(RA, ~RB)` if Z set. Updates Z. |
| NCC | 1 | 10 | `RC = NAND(RA, ~RB)` if C set. Updates Z. |
| — | 1 | 11 | Reserved / unused (no carry-chain in bitwise NAND). |

### Immediate / memory / control — I and J type
| Mnemonic | Opcode | Format | Assembly | Semantics |
|---|---|---|---|---|
| ADI | 0000 | I | `adi rb, ra, imm6` | `rb = ra + sign_extend(imm6)`. Updates C, Z. |
| LLI | 0011 | J | `lli ra, imm9` | `ra = zero_extend(imm9)` (upper 7 bits cleared). |
| LW  | 0100 | I | `lw ra, rb, imm6` | `ra = MEM[rb + sign_extend(imm6)]`. Updates Z. |
| SW  | 0101 | I | `sw ra, rb, imm6` | `MEM[rb + sign_extend(imm6)] = ra`. |
| LM  | 0110 | J | `lm ra, mask8` | Loads registers selected by an 8-bit mask (1 bit/register, R7→R0 order) from consecutive addresses starting at `ra`. ⚠ verify exact bit-to-register mapping against your old report. |
| SM  | 0111 | J | `sm ra, mask8` | Stores registers selected by mask8 to consecutive addresses starting at `ra`. Same caveat as LM. |
| BEQ | 1000 | I | `beq ra, rb, imm6` | If `ra == rb`: `PC = PC_of_this_instruction + imm6*2`. |
| BLT | 1001 | I | `blt ra, rb, imm6` | If `ra < rb`: same branch target calc as BEQ. |
| BLE | 1010 | I | `ble ra, rb, imm6` | If `ra <= rb`: same branch target calc as BEQ. **Opcode collides with BLT in the assignment PDF — confirm the real value from your old report.** |
| JAL | 1100 | J | `jal ra, imm9` | `ra = PC_of_this_instruction + 2`; `PC = PC_of_this_instruction + imm9*2`. (PDF's "jalr" in this row is a typo.) |
| JLR | 1101 | I | `jlr ra, rb` | `ra = PC_of_this_instruction + 2`; `PC = rb`. Imm field unused (zero). |
| JRI | 1111 | J | `jri ra, imm9` | `PC = ra + imm9*2`. Note: relative to the *value in ra*, not to the current PC — easy to confuse with JAL. |

## Assembler-level conventions 

| Decision | Rule |
|---|---|
| **Mnemonic case** | Case-insensitive (`ADI` = `adi`) |
| **Register case** | Case-insensitive (`R1` = `r1`) |
| **Label case** | Case-sensitive (`loop` ≠ `Loop`) |
| **Comment leader** | `;` to end of line (completely discarded by lexer) |
| **Negative numbers** | `-` immediately followed by a digit is part of the `IMMEDIATE` token |
| **Numeric Bases** | Supports Decimal (`-5`, `42`) and Hexadecimal (`0x2A`, `0xff`) |
| **Label colon spacing** | Tokenized independently (whitespace between a label and `:` is ignored) |
| **Word Priority** | Always consume the maximum alphanumeric string first, *then* match against the opcode/register keyword table. |

## Pipeline hazard notes — TODO Day 15
(Fill in once you re-derive your hazard/forwarding rules from your old VHDL project.)