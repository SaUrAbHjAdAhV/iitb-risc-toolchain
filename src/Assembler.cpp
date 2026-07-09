#include "../include/Assembler.hpp"
#include <stdexcept>
#include <iostream>
#include <fstream>  // REQUIRED FOR std::ofstream
#include <iomanip>  // REQUIRED FOR std::setw AND std::setfill

//Helper fuctions
static uint8_t getOpcodeValue(Opcode op) {
    switch (op) {
        // ADD Family
        case Opcode::ADA: return 0b0001;
        case Opcode::ADZ: return 0b0001;
        case Opcode::ADC: return 0b0001;
        case Opcode::AWC: return 0b0001;
        case Opcode::ACA: return 0b0001;
        case Opcode::ACZ: return 0b0001;
        case Opcode::ACC: return 0b0001;
        case Opcode::ACW: return 0b0001;

        // NAND Family
        case Opcode::NDU: return 0b0010;
        case Opcode::NDZ: return 0b0010;
        case Opcode::NDC: return 0b0010;
        case Opcode::NCU: return 0b0010;
        case Opcode::NCZ: return 0b0010;
        case Opcode::NCC: return 0b0010;

        // I-Type & J-Type Operations
        case Opcode::ADI: return 0b0000;
        case Opcode::LLI: return 0b0011;
        case Opcode::LW:  return 0b0100;
        case Opcode::SW:  return 0b0101;
        case Opcode::LM:  return 0b0110;
        case Opcode::SM:  return 0b0111;
        case Opcode::BEQ: return 0b1000;
        case Opcode::BLT: return 0b1001;
        case Opcode::BLE: return 0b1010;
        case Opcode::JAL: return 0b1100;
        case Opcode::JLR: return 0b1101;
        case Opcode::HALT: return 0b1110;
        case Opcode::JRI: return 0b1111;

        default: 
            throw std::runtime_error("Assembler Error: invalid opcode.");
    }
}

static uint8_t getComplementBit(Opcode op) {
    switch (op) {
        case Opcode::ACA: case Opcode::ACC: case Opcode::ACZ: case Opcode::ACW:
        case Opcode::NCU: case Opcode::NCC: case Opcode::NCZ:
            return 1; 
        default: 
            return 0; 
    }
}

static uint8_t getCZField(Opcode op) {
    switch (op) {
        case Opcode::ADZ: case Opcode::NDZ: case Opcode::ACZ: case Opcode::NCZ: return 0b01; 
        case Opcode::ADC: case Opcode::NDC: case Opcode::ACC: case Opcode::NCC: return 0b10; 
        case Opcode::AWC: case Opcode::ACW:                                      return 0b11; 
        default:                                                                  return 0b00; 
    }
}

// Constructor binds the reference to the parsed lines
Assembler::Assembler(std::vector<ParsedLine>& lines) : lines(lines) {}

// Pass 1: Maps out the program layout and records label memory slots
void Assembler::pass1() {
    uint16_t address = 0; // The hardware Program Counter starts at byte 0 

    for (auto& line : lines) {
        // Every instruction line is stamped with its location before label verification 
        line.address = address; 

        // If the line defines a label (e.g., "loop:"), record its location 
        if (line.label.has_value()) {
            const std::string& name = line.label.value(); 
            
            // Defensive Check: Catch duplicate label definitions early 
            if (symbolTable.count(name)) {
                throw std::runtime_error("Assembler Error: Duplicate label definition '" + name + 
                                         "' encountered on line " + std::to_string(line.line)); 
            }
            
            // Map the label name to the current memory address 
            symbolTable[name] = address;
        }

        // Only advance the program memory allocation if an actual execution opcode exists 
        if (line.hasInstruction) {
            address += 2; // Every instruction word takes exactly 2 bytes (16 bits) in memory 
        }
    }
}

// encodeInstruction for those with all labels resolved
uint16_t Assembler::encodeInstruction(ParsedLine& line, bool allowUnresolved) {

    // HALT instr
    if (line.op == Opcode::HALT) {
        return (0b1110 & 0xF) << 12; // 0xE000
    }
    uint16_t machineWord = 0;

    switch (line.format) {
        case InstrFormat::R: {
            // TODO: 
            // 1. Fetch 4-bit Opcode value 
            // 2. Extract register fields (RA, RB, RC) from line.operands 
            // 3. Compute the structural Complement bit and CZ status bits 
            // 4. Shift and OR fields into machineWord 
            uint16_t opCodeBits = getOpcodeValue(line.op);
            uint16_t ra = line.operands[0].regNum;
            uint16_t rb = line.operands[1].regNum;
            uint16_t rc = line.operands[2].regNum;
            uint16_t comp = getComplementBit(line.op);
            uint16_t cz = getCZField(line.op);

            // HARDENING CRITICAL: Defensive Register Validation Check
            if (ra > 7 || rb > 7 || rc > 7) {
                throw std::runtime_error("Line " + std::to_string(line.line) + 
                    ": Assembler Error: Register out of range (Allowed R0-R7). Found R" + 
                    std::to_string(ra) + ", R" + std::to_string(rb) + ", R" + std::to_string(rc));
            }

            // Pack fields into the 16-bit word safely using bitwise operations
            machineWord |= (opCodeBits & 0x0F) << 12; // Mask to 4 bits
            machineWord |= (ra & 0x07) << 9;          // Mask to 3 bits
            machineWord |= (rb & 0x07) << 6;          // Mask to 3 bits
            machineWord |= (rc & 0x07) << 3;          // Mask to 3 bits
            machineWord |= (comp & 0x01) << 2;        // Mask to 1 bit
            machineWord |= (cz & 0x03) << 0;          // Mask to 2 bits
            break;
        }
        case InstrFormat::I: {
            // TODO: 
            // 1. Fetch 4-bit Opcode value 
            // 2. Extract register fields (RA, RB) 
            // 3. Extract or calculate the 6-bit Immediate field (Imm6) 
            //    CRITICAL: If it's a LABEL_REF, compute the PC-relative offset:
            //    int16_t offset = target_address - line.address; 
            //    int16_t imm6 = offset / 2; 
            uint16_t opCodeBits = getOpcodeValue(line.op);
            uint16_t ra = line.operands[0].regNum;
            uint16_t rb = line.operands[1].regNum;

            // HARDENING CRITICAL: Defensive Register Validation Check
            if (ra > 7 || rb > 7) {
                throw std::runtime_error("Line " + std::to_string(line.line) + 
                    ": Assembler Error: Register out of range (Allowed R0-R7). Found R" + 
                    std::to_string(ra) + ", R" + std::to_string(rb));
            }

            machineWord |= (opCodeBits & 0x0F) << 12; // Mask to 4 bits
            machineWord |= (ra & 0x07) << 9;          // Mask to 3 bits
            machineWord |= (rb & 0x07) << 6;          // Mask to 3 bits
            if(line.op == Opcode::JLR){
                // JLR has no immediate field, lower 6 bits stay 0.
                // machineWord |= 0x00; is a no-op, so we can just leave it clean.
            }
            else if(line.operands[2].kind == Operand::Kind::LABEL_REF){
                if (symbolTable.count(line.operands[2].labelName) == 0) {
                    // Critical Fix: Only allow unresolved names if explicitly requested by assembleToObject
                    if (allowUnresolved) {
                        machineWord |= 0x0000; // Emit placeholder bits safely [cite: 27]
                    } else {
                        throw std::runtime_error("Line " + std::to_string(line.line) + 
                            ": Assembler Error: Reference to undefined label '" + line.operands[2].labelName + "'");
                    }
                }
                else{
                    uint16_t target_addr = symbolTable.at(line.operands[2].labelName);
                    int16_t offset = (int16_t)(target_addr - line.address);

                    // Safety check for unaligned target offsets
                    if (offset % 2 != 0) {
                        throw std::runtime_error("Line " + std::to_string(line.line) + 
                            ": Assembler Error: Unaligned branch target address offset (" + std::to_string(offset) + " bytes).");
                    }

                    int16_t imm = offset/2;
                    if (imm > 31 || imm < -32) { 
                        throw std::runtime_error("Line " + std::to_string(line.line) + ": 6 bit IMM out of bounds (-32 to 31)."); 
                    }
                    machineWord |= (imm & 0x3F);
                }
            }
            else{
                int16_t imm = line.operands[2].immValue;
                if (imm > 31 || imm < -32) { 
                    throw std::runtime_error("Line " + std::to_string(line.line) + ": 6 bit IMM out of bounds (-32 to 31)."); 
                }
                machineWord |= (imm & 0x3F);
            }
            break;
        }
        case InstrFormat::J: {
            // TODO: 
            // 1. Fetch 4-bit Opcode value 
            // 2. Extract register field (RA) 
            // 3. Extract the 9-bit Immediate field (Imm9) 
            //    Remember to look out for whether the operation is standard (LLI) 
            //    or a vector mask configuration block (LM/SM). 
            uint16_t opCodeBits = getOpcodeValue(line.op);
            uint16_t ra = line.operands[0].regNum;

            // HARDENING CRITICAL: Defensive Register Validation Check
            if (ra > 7) {
                throw std::runtime_error("Line " + std::to_string(line.line) + 
                    ": Assembler Error: Register out of range (Allowed R0-R7). Found R" + std::to_string(ra));
            }

            machineWord |= (opCodeBits & 0x0F) << 12; // Mask to 4 bits
            machineWord |= (ra & 0x07) << 9;          // Mask to 3 bits

            if (line.op == Opcode::LM || line.op == Opcode::SM) {
                int16_t imm = (int16_t)line.operands[1].immValue;
                // FIX: Ensure the 8-bit vector mask is strictly unsigned 0 to 255
                if (imm < 0 || imm > 255) {
                    throw std::runtime_error("Line " + std::to_string(line.line) + ": Vector mask out of bounds (0 to 255).");
                }
                machineWord |= (imm & 0xFF); // Mask cleanly to 8 bits
            }
            else if (line.operands[1].kind == Operand::Kind::LABEL_REF) {
                if (symbolTable.count(line.operands[1].labelName) == 0) {
                    // Critical Fix: Only allow unresolved names if explicitly requested by assembleToObject
                    if (allowUnresolved) {
                        machineWord |= 0x0000; // Emit placeholder bits safely
                    } else {
                        throw std::runtime_error("Line " + std::to_string(line.line) + 
                            ": Assembler Error: Reference to undefined label '" + line.operands[1].labelName + "'");
                    }
                }
                else{
                    uint16_t target_addr = symbolTable.at(line.operands[1].labelName);
                    int16_t offset = (int16_t)(target_addr - line.address);

                    // Safety check for unaligned target offsets
                    if (offset % 2 != 0) {
                        throw std::runtime_error("Line " + std::to_string(line.line) + 
                            ": Assembler Error: Unaligned branch target address offset (" + std::to_string(offset) + " bytes).");
                    }

                    int16_t imm = offset / 2;
                    
                    // 9-bit signed limit check (-256 to 255)
                    if (imm > 255 || imm < -256) { 
                        throw std::runtime_error("Line " + std::to_string(line.line) + ": Branch offset out of bounds (-256 to 255)."); 
                    }
                    machineWord |= (imm & 0x01FF); // Mask cleanly to 9 bits
                }
            }
            else {
                int16_t imm = (int16_t)line.operands[1].immValue;
                
                if (line.op == Opcode::LLI) {
                    // LLI Special Rule: Accept either signed (-256 to 255) OR unsigned (0 to 511)
                    // This means the raw value must simply fit within 9 bits.
                    if (line.operands[1].immValue < -256 || line.operands[1].immValue > 511) {
                        throw std::runtime_error("Line " + std::to_string(line.line) + 
                            ": LLI Immediate value out of 9-bit range (-256 to 511).");
                    }
                } else {
                    // Standard J-type 9-bit signed limit check (-256 to 255)
                    if (imm > 255 || imm < -256) { 
                        throw std::runtime_error("Line " + std::to_string(line.line) + 
                            ": Immediate value out of bounds (-256 to 255)."); 
                    }
                }
                machineWord |= (imm & 0x01FF); // Mask cleanly to 9 bits
            }
            break;
        }
        default:
            throw std::runtime_error("Line " + std::to_string(line.line) + ": Untranslatable instruction layout.");
    }

    return machineWord;
}


// Pass 2: Translates instructions into 16-bit binary machine words
std::vector<uint16_t> Assembler::pass2() {
    std::vector<uint16_t> binaryOutput;

    for (auto& line : lines) {
        // Skip lines that don't have an instruction opcode (like standalone labels)
        if (!line.hasInstruction) continue;

        uint16_t machineWord = 0;

        binaryOutput.push_back(encodeInstruction(line, false));
    }

    return binaryOutput;
}

ObjectFile Assembler::assembleToObject(const std::string& sourceFile) {
    // Run pass1 normally — builds symbol table for THIS file's labels
    pass1();

    ObjectFile obj;

    uint16_t codeOffset = 0;
    for (auto& line : lines) {
        if (!line.hasInstruction) continue;

        // Fast escape for HALT since it never needs cross-file relocation patching
        if (line.op == Opcode::HALT) {
            obj.code.push_back(encodeInstruction(line));
            codeOffset += 2;
            continue;
        }

        // Try to encode the instruction
        // If all operands are resolved (no external label refs), encode normally
        // If there's an unresolved label ref, emit placeholder + add relocation

        bool needsRelocation = false;
        std::string unresolvedSymbol;
        uint8_t relocType = 0;

        // Check if any operand is an unresolved label reference
        for (auto& operand : line.operands) {
            if (operand.kind == Operand::Kind::LABEL_REF) {
                if (symbolTable.count(operand.labelName) == 0) {
                    // This label isn't defined in this file — needs relocation
                    needsRelocation = true;
                    unresolvedSymbol = operand.labelName;
                    // Determine type based on instruction format
                    relocType = (line.format == InstrFormat::I) ? 1 : 0;
                }
            }
        }

        if (needsRelocation) {
            // Pass true to allowUnresolved to bypass strict validation
            uint16_t placeholderWord = encodeInstruction(line, true); 
            obj.code.push_back(placeholderWord);
            
            obj.relocations.push_back({
                codeOffset,
                relocType,
                unresolvedSymbol
            }); 
        } else {
            // Fully resolved locally — defaults to strict validation mode
            obj.code.push_back(encodeInstruction(line, false));
        }

        codeOffset += 2;
    }

    // Export all labels defined in this file
    for (auto& [name, addr] : symbolTable) {
        obj.symbols.push_back({name, addr});
    }

    return obj;
}

void Assembler::writeHex(const std::vector<uint16_t>& words, const std::string& outputPath) {
    std::ofstream out(outputPath);
    if (!out.is_open()) {
        throw std::runtime_error("Cannot open output file: " + outputPath);
    }

    uint16_t address = 0;
    for (uint16_t word : words) {
        uint8_t highByte = (word >> 8) & 0xFF;
        uint8_t lowByte  =  word       & 0xFF;

        uint8_t addrHigh = (address >> 8) & 0xFF;
        uint8_t addrLow  =  address       & 0xFF;

        // Compute checksum over: byteCount (0x02), addrHigh, addrLow, recordType (0x00), data bytes
        uint8_t sum = 0x02 + addrHigh + addrLow + 0x00 + highByte + lowByte;
        uint8_t checksum = (~sum + 1) & 0xFF; // Two's complement representation

        out << ":"
            << std::uppercase << std::hex << std::setfill('0')
            << std::setw(2) << 0x02
            << std::setw(4) << address
            << "00"
            << std::setw(2) << (int)highByte
            << std::setw(2) << (int)lowByte
            << std::setw(2) << (int)checksum
            << "\n";

        address += 2;
    }

    // Standard Intel HEX End of File (EOF) record
    out << ":00000001FF\n";
}