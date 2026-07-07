#include "../include/Emulator.hpp"
#include <iostream>
#include <iomanip>
#include <stdexcept>

// ISOLATED CORE DECODING UTILITIES & EXTENSION HELPERS
static inline uint8_t  fieldOpcode(uint16_t i)     { return (i >> 12) & 0xF; }   
static inline uint8_t  fieldRA(uint16_t i)         { return (i >>  9) & 0x7; }   
static inline uint8_t  fieldRB(uint16_t i)         { return (i >>  6) & 0x7; }   
static inline uint8_t  fieldRC(uint16_t i)         { return (i >>  3) & 0x7; }   
static inline uint8_t  fieldComp(uint16_t i)       { return (i >>  2) & 0x1; }  
static inline uint8_t  fieldCZ(uint16_t i)         { return  i        & 0x3; }   

// Sign-extend a raw bit-field value from n bits up to a full signed 16-bit word
static inline int16_t signExtend(uint16_t val, int bits) {
    uint16_t mask = 1 << (bits - 1);                                             
    return (val ^ mask) - mask;                                                  
}

// Imm6 sign-extended extraction (I-type instructions)
static inline int16_t fieldImm6(uint16_t i) {
    return signExtend(i & 0x3F, 6);                                              
}

// Imm9 raw zero-extended extraction (J-type instructions)
static inline uint16_t fieldImm9raw(uint16_t i)    { return i & 0x1FF; }         

// Imm9 sign-extended extraction (J-type jumps)
static inline int16_t  fieldImm9signed(uint16_t i) {
    return signExtend(i & 0x1FF, 9);                                             
}

Emulator::Emulator() {
    reg.fill(0);
    flagC = false;
    flagZ = false;
    halted = false;
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
    // Reset state flag whenever a new program execution runtime starts
    halted = false;

    for (uint32_t cycle = 0; cycle < maxCycles; cycle++) {
        // Stop execution if PC points out of valid space or hits a NOP boundary halt
        if (reg[0] >= 65534 || halted){
            break;
        } 

        // Save the EXACT address of the current instruction before PC gets advanced!
        uint16_t currentPC = reg[0];
        
        uint16_t instr = fetch();
        
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

// INSTRUCTION FETCH ENTRY ENGINE
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

void Emulator::updateFlags(uint32_t result) {
    // Re-evaluate flag conditions using wide 32-bit parameters to track overflow boundaries
    flagZ = ((result & 0xFFFF) == 0);                                            
    flagC = (result > 0xFFFF);                                                   
}

void Emulator::execute(uint16_t instr, uint16_t currentPC) {
    uint8_t opcode = fieldOpcode(instr);                                         
    switch (opcode) {
        case 0b0000:{ // ADI
            uint8_t rb = fieldRB(instr);
            int16_t imm6 = fieldImm6(instr);
            uint32_t accumulator = static_cast<uint32_t>(reg[rb]) + static_cast<uint32_t>(static_cast<int32_t>(imm6));
            updateFlags(accumulator);
            reg[rb] = static_cast<uint16_t>(accumulator & 0xFFFF);
            break;
        }
        case 0b0001: // ADD Family
            execADDFamily(instr);
            break;
        case 0b0010: // NAND Family
            execNandFamily(instr);
            break;
        case 0b0011:{ //LLI
            uint8_t ra = fieldRA(instr);
            reg[ra] = fieldImm9raw(instr);
            break;
        }
        case 0b0100:{ //LW
            uint8_t  ra   = fieldRA(instr);
            uint8_t  rb   = fieldRB(instr);
            int16_t  imm6 = fieldImm6(instr);

            uint16_t address = static_cast<uint16_t>(static_cast<int32_t>(reg[rb]) + imm6);
            if(address > 65534){
                throw std::runtime_error("LW: memory address out of bounds");
            }
            reg[ra] = (memory[address] << 8) | memory[address+1];
            break;
        }
        case 0b0101:{ //SW
            uint8_t  ra   = fieldRA(instr);
            uint8_t  rb   = fieldRB(instr);
            int16_t  imm6 = fieldImm6(instr);

            uint16_t address = static_cast<uint16_t>(static_cast<int32_t>(reg[rb]) + imm6);
            if(address > 65534){
                throw std::runtime_error("SW: memory address out of bounds");
            }
            memory[address]     = (reg[ra] >> 8) & 0xFF; 
            memory[address + 1] = reg[ra] & 0xFF;
            break;
        }
        case 0b0110:{ //LM
            uint8_t ra     = fieldRA(instr);
            uint8_t mask   = static_cast<uint8_t>(instr & 0xFF); // lower 8-bit mask

            uint16_t address = reg[ra];

            for(int bit = 0; bit < 8; bit++){
                if(mask & (1<<bit)){
                    if (address > 65534) {
                        throw std::runtime_error("LM: Memory address out of boundaries!");
                    }
                    reg[7-bit] = ((uint16_t)memory[address] << 8) | memory[address + 1];
                    address += 2;
                }
            }
            break;
        }
        case 0b0111:{ //SM
            uint8_t ra     = fieldRA(instr);
            uint8_t mask   = static_cast<uint8_t>(instr & 0xFF); // lower 8-bit mask

            uint16_t address = reg[ra];

            for(int bit = 0; bit < 8; bit++){
                if(mask & (1<<bit)){
                    if (address > 65534) {
                        throw std::runtime_error("SM: Memory address out of boundaries!");
                    }
                    memory[address] = (reg[7-bit] >> 8) & 0xFF;
                    memory[address+1] = reg[7-bit] & 0xFF;
                    address += 2;
                }
            }
            break;
        }
        case 0b1000:{ //BEQ
            uint8_t  ra   = fieldRA(instr);
            uint8_t  rb   = fieldRB(instr);
            int16_t  imm6 = fieldImm6(instr);

            if(reg[ra] == reg[rb]){
                uint16_t address = (uint16_t)(currentPC + 2*imm6);
                if(address > 65534){
                    throw std::runtime_error("BEQ: PC address out of bounds");
                }
                reg[0] = address;
            }
            break;
        }
        case 0b1001:{ //BLT
            uint8_t  ra   = fieldRA(instr);
            uint8_t  rb   = fieldRB(instr);
            int16_t  imm6 = fieldImm6(instr);

            int16_t reg_a_data = (int16_t)(reg[ra]);
            int16_t reg_b_data = (int16_t)(reg[rb]);
            if(reg_a_data < reg_b_data){
                uint16_t address = (uint16_t)(currentPC + 2*imm6);
                if(address > 65534){
                    throw std::runtime_error("BLT: PC address out of bounds");
                }
                reg[0] = address;
            }
            break;
        }
        case 0b1010:{ //BLE
            uint8_t  ra   = fieldRA(instr);
            uint8_t  rb   = fieldRB(instr);
            int16_t  imm6 = fieldImm6(instr);

            int16_t reg_a_data = (int16_t)(reg[ra]);
            int16_t reg_b_data = (int16_t)(reg[rb]);
            if(reg_a_data <= reg_b_data){
                uint16_t address = (uint16_t)(currentPC + 2*imm6);
                if(address > 65534){
                    throw std::runtime_error("BLE: PC address out of bounds");
                }
                reg[0] = address;
            }
            break;   
        }
        case 0b1100: { // JAL
            uint8_t ra       = fieldRA(instr);
            int16_t imm9_sig = fieldImm9signed(instr);

            uint16_t target = static_cast<uint16_t>(static_cast<int32_t>(currentPC) + (imm9_sig * 2)); // 
            if (target > 65534) {
                throw std::runtime_error("JAL: Target address out of unified memory boundaries!");
            }

            reg[ra] = currentPC + 2;
            reg[0] = target;
            break;
        }
        case 0b1101: { // JLR
            uint8_t ra = fieldRA(instr);
            uint8_t rb = fieldRB(instr);

            // Cache the jump target address immediately before touching any registers!
            uint16_t target = reg[rb]; 
            if (target > 65534) {
                throw std::runtime_error("JLR: Target address out of unified memory boundaries!");
            }

            reg[ra] = currentPC + 2;
            reg[0] = target;            
            break;
        }
        case 0b1111: { // JRI
            uint8_t ra       = fieldRA(instr);
            int16_t imm9_sig = fieldImm9signed(instr);

            // JRI does NOT link. Target is base register + immediate
            uint16_t target = static_cast<uint16_t>(static_cast<int32_t>(reg[ra]) + (imm9_sig * 2)); 
            if (target > 65534) {
                throw std::runtime_error("JRI: Target address out of unified memory boundaries!");
            }

            reg[0] = target;
            break;
        }
        case 0b1110: { // HALT
            // Program terminator 
            halted = true;
            return;
        }
        default:
            throw std::runtime_error(
                "Unknown opcode 0x" + std::to_string(opcode)
                + " at PC=0x" + std::to_string(currentPC));
            break;
    }
}

// ADD Family Execution
void Emulator::execADDFamily(uint16_t instr) {
    // Extract instruction subfields using bit shifting and bitwise masking
    uint8_t dest_ra   = fieldRA(instr);
    uint8_t src_rb   = fieldRB(instr);
    uint8_t src_rc   = fieldRC(instr);
    uint8_t comp = fieldComp(instr);
    uint8_t cz   = fieldCZ(instr);

    // Gated Predication (Condition Code Check)

    bool shouldexecute = false;

    switch (cz) {
        case 0b00:
            shouldexecute = true;
            break;
        case 0b01:
            shouldexecute = flagZ;
            break;
        case 0b10:
            shouldexecute = flagC;
            break;
        case 0b11:
            shouldexecute = true;
            break;
    }

    if(!shouldexecute) return;

    // Operand Preparation & Complement Logic

    uint16_t opA = reg[src_rb];
    uint16_t opB = comp? ~reg[src_rc] : reg[src_rc];

    // Wide Arithmetic & Carry-In Integration

    uint32_t accumulator = static_cast<uint32_t>(opA) + static_cast<uint32_t>(opB);
    if(cz == 0b11) accumulator += (uint32_t)(flagC? 1 : 0);
    updateFlags(accumulator);
    reg[dest_ra] = static_cast<uint16_t>(accumulator & 0xFFFF);
}

//NAND Family Execution
void Emulator::execNandFamily(uint16_t instr){
    uint8_t dest_ra   = fieldRA(instr);
    uint8_t src_rb   = fieldRB(instr);
    uint8_t src_rc   = fieldRC(instr);
    uint8_t comp = fieldComp(instr);
    uint8_t cz   = fieldCZ(instr);

    bool shouldexecute = false;

    switch (cz) {
        case 0b00:
            shouldexecute = true;
            break;
        case 0b01:
            shouldexecute = flagZ;
            break;
        case 0b10:
            shouldexecute = flagC;
            break;
        default:
            break;
    }

    if(!shouldexecute) return;

    uint16_t opA = reg[src_rb];
    uint16_t opB = comp? ~reg[src_rc] : reg[src_rc];

    uint16_t result = ~(opA & opB);
    flagZ = ((result & 0xFFFF) == 0);
    reg[dest_ra] = result;

}