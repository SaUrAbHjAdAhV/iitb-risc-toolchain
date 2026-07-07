#pragma once
#include <cstdint>
#include <array>
#include <vector>
#include <string>

class Emulator {
public:
    Emulator();
    
    // Loads the compiled 16-bit binary payload directly into execution memory
    void loadProgram(const std::vector<uint16_t>& words, uint16_t startAddress = 0);
    
    // Core hardware cycle execution loop
    void run(uint32_t maxCycles = 100000);
    
    // Diagnostic state output log (prints registers and condition flags)
    void dumpState() const;

private:
    // Core Hardware Architecture State
    std::array<uint16_t, 8> reg;  // reg[0] acts as the Program Counter (PC), reg[1]-reg[7] general purpose 
    bool flagC;                    // Carry condition flag register status 
    bool flagZ;                    // Zero condition flag register status 
    bool halted;                   // Track if the processor has received a terminal HALT signal
    std::array<uint8_t, 65536> memory; // Complete 16-bit unified big-endian byte address space 

    // Flag management utility
    void updateFlags(uint32_t result);

    // Execution Execution Pass Steps
    uint16_t fetch();
    void execute(uint16_t instruction, uint16_t currentPC);

    // Format Class Sub-decoders
    void execADDFamily(uint16_t instr);
    void execNandFamily(uint16_t instr);
    void execIType(uint16_t instr, uint16_t currentPC);
    void execJType(uint16_t instr, uint16_t currentPC);
};