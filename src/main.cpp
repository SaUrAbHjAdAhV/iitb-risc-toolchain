#include "../include/Lexer.hpp"
#include "../include/Parser.hpp"
#include "../include/Assembler.hpp"
#include "../include/Emulator.hpp"
#include "../include/ObjectFile.hpp"
#include "../include/Linker.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

// Helper routine to read flat source assembly text from disk
std::string readSourceFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("File Stream Exception: Could not open source assembly file '" + path + "'");
    }
    std::stringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "IITB-RISC Toolchain Usage Rules:\n"
                  << "  1. Direct Execute Mode : ./risc_toolchain <file.asm>\n"
                  << "  2. Compile-Only Mode   : ./risc_toolchain -c <file.asm>\n"
                  << "  3. Static Linker Mode  : ./risc_toolchain -o <out.hex> <file1.obj> <file2.obj> ...\n";
        return 1;
    }

    std::string firstArg = argv[1];

    // =========================================================================
    // MODE 2: Compile-Only Mode (-c flag) -> Produces an independent .obj file
    // =========================================================================
    if (firstArg == "-c") {
        if (argc < 3) {
            std::cerr << "Error: Compile-only mode requires an input assembly source file (.asm).\n";
            return 1;
        }
        std::string sourcePath = argv[2];

        try {
            std::string sourceText = readSourceFile(sourcePath);

            Lexer lexer(sourceText);
            auto tokens = lexer.scanTokens();

            Parser parser(tokens);
            auto parsedLines = parser.parse();

            Assembler assembler(parsedLines);
            // Run Pass 1 and Pass 2 in relocatable compilation mode
            ObjectFile obj = assembler.assembleToObject(sourcePath);

            // Derive the output file path by replacing extension with .obj
            std::string objPath = sourcePath;
            auto dotPos = objPath.rfind('.');
            if (dotPos != std::string::npos) {
                objPath = objPath.substr(0, dotPos);
            }
            objPath += ".obj";

            writeObjectFile(obj, objPath);
            std::cout << "Compilation Successful: Relocatable object file written to: " << objPath << "\n";

        } catch (const std::exception& err) {
            std::cerr << "\n[COMPILATION FAILED]: " << err.what() << "\n";
            return 1;
        }
    }
    // =========================================================================
    // MODE 3: Static Linker Mode (-o flag) -> Combines multiple .obj files into a .hex
    // =========================================================================
    else if (firstArg == "-o") {
        if (argc < 4) {
            std::cerr << "Error: Linker mode usage requires: -o <out.hex> <file1.obj> <file2.obj> ...\n";
            return 1;
        }
        std::string outputPath = argv[2];
        
        Linker linker;

        try {
            // Read every object file path supplied in the argument list trailing the output path
            for (int i = 3; i < argc; ++i) {
                std::string objPath = argv[i];
                ObjectFile obj = readObjectFile(objPath);
                linker.addObject(obj);
            }

            // Run the cross-file macro patching loops
            auto finalBinaryWords = linker.link();

            // Feed dummy reference lines to utilize the assembler's existing writeHex wrapper layout
            std::vector<ParsedLine> dummyLines;
            Assembler hexEmitter(dummyLines);
            hexEmitter.writeHex(finalBinaryWords, outputPath);

            std::cout << "Linkage Process Successful: Consolidated binary executable written to: " << outputPath << "\n";

        } catch (const std::exception& err) {
            std::cerr << "\n[LINKAGE REJECTION DETECTED]: " << err.what() << "\n";
            return 1;
        }
    }
    // =========================================================================
    // MODE 1: Direct Execute Mode (Default) -> Legacy compilation + emulator execution pass
    // =========================================================================
    else {
        std::string sourcePath = firstArg;

        try {
            std::string sourceText = readSourceFile(sourcePath);

            Lexer lexer(sourceText);
            auto tokens = lexer.scanTokens();

            Parser parser(tokens);
            auto parsedLines = parser.parse();

            Assembler assembler(parsedLines);
            assembler.pass1();
            auto binaryWords = assembler.pass2();

            std::string hexPath = sourcePath;
            auto dotPos = hexPath.rfind('.');
            if (dotPos != std::string::npos) {
                hexPath = hexPath.substr(0, dotPos);
            }
            hexPath += ".hex";

            assembler.writeHex(binaryWords, hexPath);

            std::cout << "Pipeline Compilation Complete! Standard output hex written to: " << hexPath << "\n";
            std::cout << "Initializing direct local verification execution runtime...\n";

            Emulator emu;
            emu.loadProgram(binaryWords, 0x0000);
            emu.run(100000);
            emu.dumpState();

        } catch (const std::exception& err) {
            std::cerr << "\n[PIPELINE TERMINATION EXCEPTION]: " << err.what() << "\n";
            return 1;
        }
    }

    return 0;
}