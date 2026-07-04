#pragma once
#include "Instruction.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>

class Assembler {
public:
    Assembler(std::vector<ParsedLine>& lines);
    
    // Pass 1: Maps out label positions and stamps memory addresses
    void pass1();
    
    // Pass 2: Generates the raw binary machine words (We will write this next)
    std::vector<uint16_t> pass2();

    void writeHex(const std::vector<uint16_t>& words, const std::string& outputPath);

private:
    std::vector<ParsedLine>& lines;
    std::unordered_map<std::string, uint16_t> symbolTable;
};