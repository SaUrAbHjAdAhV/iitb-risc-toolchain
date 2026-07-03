#include "../include/Lexer.hpp"
#include "../include/Parser.hpp"
#include "../include/Assembler.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

int main(int argc, char* argv[]) {
    // 1. Validate that the user provided a source file argument
    if (argc < 2) {
        std::cerr << "Usage: assembler <file.asm>\n";
        return 1;
    }

    // 2. Read raw assembly text from file disk
    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open assembly file '" << argv[1] << "'\n";
        return 1;
    }

    std::stringstream buf;
    buf << file.rdbuf();
    std::string sourceText = buf.str();

    try {
        // 3. Pipeline Pass 0: Lexical Tokenization
        Lexer lexer(sourceText);
        auto tokens = lexer.scanTokens();

        // 4. Pipeline Pass 0.5: Syntactic Grammar Parsing
        Parser parser(tokens);
        auto parsedLines = parser.parse();

        // 5. Pipeline Pass 1: Structural Address Stamping & Symbol Table Mapping
        Assembler assembler(parsedLines);
        assembler.pass1();

        // 6. Pipeline Pass 2: Machine Bit Packing Emission
        auto binaryWords = assembler.pass2();

        // 7. Temporary Terminal Output (We will write to a file on Day 4)
        std::cout << "\n--- ASSEMBLER BINARY EMISSION (HEX) ---\n";
        for (size_t i = 0; i < binaryWords.size(); i++) {
            std::cout << std::hex << std::setw(4) << std::setfill('0') << binaryWords[i] << "\n";
        }
        std::cout << "---------------------------------------\n";
        std::cout << "Assembly compilation successful! Generated " << binaryWords.size() << " instruction words.\n";

    } catch (const std::runtime_error& err) {
        std::cerr << "\n[COMPILATION FAILED]\n" << err.what() << "\n";
        return 1;
    }

    return 0;
}