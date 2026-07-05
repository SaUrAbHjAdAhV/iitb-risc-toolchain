#include "../include/Lexer.hpp"
#include "../include/Parser.hpp"
#include "../include/Assembler.hpp"
#include "../include/Emulator.hpp"
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

        // 7. Output Pipeline Generation: Write out structural Intel HEX format
        std::string outputFile = std::string(argv[1]);
        
        // Replace input file extension with .hex extension
        auto dotPos = outputFile.rfind('.');
        if (dotPos != std::string::npos) {
            outputFile = outputFile.substr(0, dotPos);
        }
        outputFile += ".hex";

        // Write binary payload block to file
        assembler.writeHex(binaryWords, outputFile);
        
        std::cout << "\n--- PIPELINE COMPILATION SUCCESSFUL ---\n";
        std::cout << "Output written to   : " << outputFile << "\n";
        std::cout << "Generated code size : " << binaryWords.size() << " instruction words.\n";
        std::cout << "----------------------------------------\n";

        // 8. Emulator Verification Pass
        std::cout << "\n--- INITIALIZING EMULATION VERIFICATION DRIVER ---\n";
        Emulator emulator;
        
        // Load the generated binary machine words into memory starting at address 0x0000
        emulator.loadProgram(binaryWords, 0x0000);
        
        std::cout << "Executing program payload...\n";
        emulator.run(100000); // Run with safety cycle limit
        
        // Dump the final register states to verify everything executed correctly
        emulator.dumpState();

    } catch (const std::runtime_error& err) {
        std::cerr << "\n[COMPILATION FAILED]\n" << err.what() << "\n";
        return 1;
    }

    return 0;
}