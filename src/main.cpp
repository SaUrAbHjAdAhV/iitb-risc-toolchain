#include "../include/Lexer.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    std::cout << "IITB-RISC-23 Assembler Toolchain (v0.1)\n";
    if (argc < 2) {
        std::cout << "Usage: risc_toolchain <source_file.asm>\n";
        return 0;
    }
    return 0;
}