// src/Linker.cpp
#include "../include/Linker.hpp"
#include <stdexcept>

void Linker::addObject(const ObjectFile& obj) {
    objects.push_back(obj);
}

std::vector<uint16_t> Linker::link() {
    // Clear out any stale state from previous linking attempts
    globalSymbolTable.clear();
    mergedCode.clear();

    // -------------------------------------------------------------------------
    // PASS 1: Base Address Assignment, Code Merging, & Global Symbol Gathering
    // -------------------------------------------------------------------------
    std::vector<uint16_t> baseAddresses; // Tracks the starting byte address of each object file [cite: 58]
    uint16_t currentBase = 0;           // Global hardware Program Counter tracking starts at byte 0 [cite: 59]

    for (auto& obj : objects) {
        baseAddresses.push_back(currentBase); // Save the base offset for Pass 2 

        // Map every exported local label of this module to its true global coordinate
        for (auto& sym : obj.symbols) {
            uint16_t globalAddr = currentBase + sym.address; // Global byte = object base + internal relative offset [cite: 60]
            
            // Hardening: Catch duplicate global definitions early (e.g., two files creating 'loop:')
            if (globalSymbolTable.count(sym.name)) {
                throw std::runtime_error("Linker Resolution Failure: Duplicate global symbol definition '" + 
                                         sym.name + "' detected across compilation units."); 
            }
            globalSymbolTable[sym.name] = globalAddr; 
        }

        // Stitch this object's raw machine code into our unified execution array
        for (uint16_t word : obj.code) {
            mergedCode.push_back(word); 
        }

        // Advance the base address pointer. Each instruction word is exactly 2 bytes.
        currentBase += (uint16_t)(obj.code.size() * 2); 
    }

    // -------------------------------------------------------------------------
    // PASS 2: Relocation Resolution & Structural Bitwise Patching
    // -------------------------------------------------------------------------
    for (size_t objIdx = 0; objIdx < objects.size(); objIdx++) {
        auto& obj  = objects[objIdx]; 
        uint16_t base = baseAddresses[objIdx]; // Retrieve this file's starting global address 

        for (auto& rel : obj.relocations) {
            // Find the exact global byte location where the hole exists
            uint16_t globalByteOffset = base + rel.codeOffset; 
            size_t   wordIndex        = globalByteOffset / 2;  // Convert to array index into mergedCode 

            // Verify the requested symbol actually exists anywhere in our global table
            if (!globalSymbolTable.count(rel.symbolName)) {
                throw std::runtime_error("Linker Resolution Failure: Undefined symbol reference '" + 
                                         rel.symbolName + "' could not be resolved."); 
            }
            
            uint16_t targetAddr = globalSymbolTable[rel.symbolName]; // The true global address of the target symbol 
            uint16_t instrAddr  = globalByteOffset;                  // The global address of the instruction being patched 

            if (rel.type == 1) {
                // Type 1: PC-Relative branch (I-type imm6 for instructions like BEQ, BLT, etc.) 
                int16_t offset = (int16_t)(targetAddr - instrAddr); // Calculate byte-accurate distance 
                int16_t imm6   = offset / 2;                        // Convert from bytes to 16-bit instruction words 
                
                // Enforce our tight 6-bit signed immediate physical boundaries [-32 to 31]
                if (imm6 < -32 || imm6 > 31) {
                    throw std::runtime_error("Linker Boundary Exception: Branch target for '" + 
                                             rel.symbolName + "' is out of range for a 6-bit signed immediate.");
                }
                
                // Clear the lower 6 bits of the placeholder instruction word and mask in the true imm6 fields
                mergedCode[wordIndex] = (mergedCode[wordIndex] & 0xFFC0) | (imm6 & 0x3F); 
            } 
            else {
                // Type 0: Absolute Jumps (J-type imm9 used by instructions like JAL, JRI) 
                int16_t imm9 = (int16_t)(targetAddr / 2); // Convert target byte address into target word address 
                
                // Enforce our 9-bit immediate storage mask
                if (imm9 < -256 || imm9 > 511) {
                    throw std::runtime_error("Linker Boundary Exception: Jump target for '" + 
                                             rel.symbolName + "' is out of range for a 9-bit instruction field.");
                }

                // Clear the lower 9 bits of the placeholder instruction word and mask in the true imm9 target
                mergedCode[wordIndex] = (mergedCode[wordIndex] & 0xFE00) | (imm9 & 0x1FF); 
            }
        }
    }

    return mergedCode; // Return the fully linked, binary-accurate machine code payload! 
}