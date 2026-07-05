#include "../include/Emulator.hpp"
#include <iostream>
#include <iomanip>
#include <stdexcept>

Emulator::Emulator() {
    reg.fill(0);
    flagC = false;
    flagZ = false;
    memory.fill(0);
}

void Emulator::loadProgram(const std::vector<uint16_t>& words, uint16_t startAddress) {
    uint16_t addr = startAddress;
    for (uint16_t word : words) {
        if (addr >= 65535) {
            throw std::runtime_error("Program overflowed unified memory boundaries!");
        }
        // Store big-endian format: High byte first, followed by low byte
        memory[addr]     = (word >> 8) & 0xFF;
        memory[addr + 1] = word & 0xFF;
        addr += 2;
    }
    reg[0] = startAddress; // Initialize PC to the program entry point
}

void Emulator::run(uint32_t maxCycles) {
    for (uint32_t cycle = 0; cycle < maxCycles; cycle++) {
        // Stop execution if PC points out of valid space or hits a NOP boundary halt
        if (reg[0] >= 65534) break;

        // Save the EXACT address of the current instruction before PC gets advanced!
        uint16_t currentPC = reg[0];
        
        uint16_t instr = fetch();
        
        // Custom halt condition: If instruction is completely zero, assume end of execution path
        if (instr == 0x0000 && reg[0] > 2) {
            break;
        }
        
        execute(instr, currentPC);
    }
}

void Emulator::dumpState() const {
    std::cout << "\n=== EMULATOR STATE SNAPSHOT ===\n";
    std::cout << "PC (R0): 0x" << std::hex << std::setw(4) << std::setfill('0') << reg[0] << "\n";
    for (int i = 1; i < 8; ++i) {
        std::cout << "R" << i << "     : 0x" << std::hex << std::setw(4) << std::setfill('0') << reg[i] << "\n";
    }
    std::cout << "Flags  : Z=" << flagZ << " C=" << flagC << "\n";
    std::cout << "================================\n";
}

// =========================================================================
// CHALLENGE 1: INSTRUCTION FETCH ENTRY ENGINE
// =========================================================================
uint16_t Emulator::fetch() {
    // TODO: 
    // 1. Read the high byte from memory at the current PC (reg[0]) address.
    // 2. Read the low byte from memory at (PC + 1).
    // 3. Reconstruct them into a single 16-bit word (high byte shifted left by 8, OR'd with low byte).
    // 4. Advance the PC (reg[0]) by 2 bytes.
    // 5. Return the completed 16-bit instruction word.
    uint16_t instr = (memory[reg[0]] << 8) | memory[reg[0]+1];

    reg[0] += 2;
    
    return instr; 
}

void Emulator::execute(uint16_t instr, uint16_t currentPC) {
    uint8_t opcode = (instr >> 12) & 0xF;
    switch (opcode) {
        case 0b0001: // ADD Family
            execRType(instr, currentPC);
            break;
        default:
            // For Day 5, we gracefully skip unhandled opcodes or throw
            break;
    }
}

// =========================================================================
// CHALLENGE 2: R-TYPE DECODER AND ADA EXECUTION
// =========================================================================
void Emulator::execRType(uint16_t instr, uint16_t currentPC) {
    // Extract instruction subfields using bit shifting and bitwise masking
    uint8_t ra   = (instr >> 9) & 0x7;
    uint8_t rb   = (instr >> 6) & 0x7;
    uint8_t rc   = (instr >> 3) & 0x7;
    uint8_t comp = (instr >> 2) & 0x1;
    uint8_t cz   = instr & 0x3;

    // For Day 5, we are strictly verifying unconditional ADA (comp=0, cz=0)
    if (comp == 0 && cz == 0) {
        // TODO:
        // 1. Perform the arithmetic addition: Source Register B + Source Register C.
        //    Hint: Use a wider integer type (like uint32_t) to check for a hardware carry out!
        // 2. Extract the lower 16 bits of the result and save it into Destination Register A (reg[ra]).
        // 3. Calculate and update the Zero flag (flagZ): true if the 16-bit result is 0, false otherwise.
        // 4. Calculate and update the Carry flag (flagC): true if the addition overflowed bit 15.
        uint32_t accumulator = static_cast<uint32_t>(reg[rb]) + static_cast<uint32_t>(reg[rc]);
        reg[ra] = static_cast<uint16_t>(accumulator & 0xFFFF);
        // Dynamic Flag Evaluation: Ensures flags flip back to false when conditions clear
        flagZ = (reg[ra] == 0);
        flagC = ((accumulator & 0x10000) != 0); // Check if bit 16 is high (overflow out of 16 bits)
    }
}