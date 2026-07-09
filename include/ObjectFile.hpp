#pragma once
#include <cstdint>
#include <string>
#include <vector>

/*
================================================================================
RISC Custom Binary Object File Format Specification
================================================================================

[Header]
  4 bytes: magic number = 0x52495343  ("RISC" in ASCII)
  2 bytes: version = 0x0001
  2 bytes: number of code words (N)
  2 bytes: number of symbol table entries (S)
  2 bytes: number of relocation entries (R)

[Code Section]
  N × 2 bytes: the assembled instruction words
  (some words have placeholder values where addresses aren't known yet)

[Symbol Table]
  S × entry:
    32 bytes: null-terminated label name (max 31 chars + null)
     2 bytes: address of this label within this object file's code

[Relocation Table]
  R × entry:
    2 bytes: offset into code section where patching is needed
    1 byte:  relocation type (0=absolute address, 1=PC-relative branch)
    32 bytes: name of the external symbol being referenced
================================================================================
*/

static constexpr uint32_t OBJ_MAGIC   = 0x52495343; // "RISC" in ASCII 
static constexpr uint16_t OBJ_VERSION = 0x0001;     // Format Version 

// Represents a label defined and exported by this file 
struct SymbolEntry {
    std::string name;
    uint16_t    address; // Offset relative to the start of this file's code section 
};

// Represents a hole in the code that needs to be patched by the linker [cite: 6, 14, 16]
struct RelocationEntry {
    uint16_t    codeOffset;   // Byte offset into the code section where patching is needed 
    uint8_t     type;         // 0 = absolute (J-type imm9), 1 = PC-relative (I-type imm6) 
    std::string symbolName;  // Name of the external symbol that needs to be resolved 
};

// The complete in-memory representation of an object file 
struct ObjectFile {
    std::vector<uint16_t>        code;        // Assembled instruction words
    std::vector<SymbolEntry>     symbols;     // Exported local labels 
    std::vector<RelocationEntry> relocations; // Unresolved references to external symbols 
};

void writeObjectFile(const ObjectFile& obj, const std::string& path);
ObjectFile readObjectFile(const std::string& path);