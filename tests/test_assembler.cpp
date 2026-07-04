#include "../include/Lexer.hpp"
#include "../include/Parser.hpp"
#include "../include/Assembler.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <cassert>

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "[TEST FAILED] " << message << "\n"; \
            exit(1); \
        } \
    } while (0)

static std::vector<uint16_t> assembleString(const std::string& source) {
    Lexer lexer(source);
    auto tokens = lexer.scanTokens();
    Parser parser(tokens);
    auto lines = parser.parse();
    Assembler assembler(lines);
    assembler.pass1();
    return assembler.pass2();
}

void run_assembler_tests() {
    std::cout << "Running Assembler Unit Tests...\n";

    // TEST A: R-type Encoding (ADD Family)
    // ADA R2, R2, R1 -> opcode=0001, RA=010, RB=010, RC=001, comp=0, CZ=00 -> 0x1488
    {
        auto words = assembleString("ADA R2, R2, R1");
        TEST_ASSERT(words[0] == 0x1488, "Test A: ADA encoding failed");
    }

    // TEST B: R-type with Complement Bit (ACA)
    // ACA R3, R1, R2 -> opcode=0001, RA=011, RB=001, RC=010, comp=1, CZ=00 -> 0x18CA
    {
        auto words = assembleString("ACA R3, R1, R2");
        TEST_ASSERT(words[0] == 0x18CA, "Test B: ACA complement encoding failed");
    }

    // TEST C: R-type with Condition Code CZ Field (ADC)
    // ADC R1, R2, R3 -> opcode=0001, RA=001, RB=010, RC=011, comp=0, CZ=10 -> 0x129A
    {
        auto words = assembleString("ADC R1, R2, R3");
        TEST_ASSERT(words[0] == 0x129A, "Test C: ADC condition encoding failed");
    }

    // TEST D: NAND Family Variant
    // NDU R1, R2, R3 -> opcode=0010, RA=001, RB=010, RC=011, comp=0, CZ=00 -> 0x2298
    {
        auto words = assembleString("NDU R1, R2, R3");
        TEST_ASSERT(words[0] == 0x2298, "Test D: NDU logic encoding failed");
    }

    // TEST E: ADI with Negative Immediate Value
    // ADI R1, R1, -1 -> opcode=0000, RA=001, RB=001, imm6=-1 -> 0x027F
    {
        auto words = assembleString("ADI R1, R1, -1");
        TEST_ASSERT(words[0] == 0x027F, "Test E: ADI negative immediate encoding failed");
    }

    // TEST F: LLI Large Immediate Upper Range
    // LLI R3, 255 -> opcode=0011, RA=011, imm9=255 -> 0x36FF
    {
        auto words = assembleString("LLI R3, 255");
        TEST_ASSERT(words[0] == 0x36FF, "Test F: LLI field encoding failed");
    }

    // TEST G: Forward Label Offset Calculation
    // BEQ R1, R0, target (at 0x0), target is at 0x4 -> offset=4 -> imm6=2 -> 0x8202
    {
        auto words = assembleString(
            "BEQ R1, R0, target\n"
            "ADA R1, R1, R2\n"
            "target:\n"
            "ADA R2, R2, R3\n"
        );
        TEST_ASSERT(words[0] == 0x8202, "Test G: Forward target calculation incorrect");
    }

    // TEST H: Backward Label Offset Loop
    // loop at 0x0, BEQ at 0x2 -> offset=-2 -> imm6=-1 -> 0x803F
    {
        auto words = assembleString(
            "loop:\n"
            "ADA R1, R1, R2\n"
            "BEQ R0, R0, loop\n"
        );
        TEST_ASSERT(words[1] == 0x803F, "Test H: Backward loop calculation incorrect");
    }

    // TEST I: Diagnostic Exception Guard for Duplicate Labels
    {
        bool caughtDuplicate = false;
        try {
            assembleString("foo:\nfoo:\nADA R1, R1, R2\n");
        } catch (const std::runtime_error&) {
            caughtDuplicate = true;
        }
        TEST_ASSERT(caughtDuplicate, "Test I: Failed to raise exception on duplicate labels");
    }

    // TEST J: Diagnostic Exception Guard for Missing Undefined Labels
    {
        bool caughtMissing = false;
        try {
            assembleString("BEQ R1, R0, ghost\n");
        } catch (const std::runtime_error&) {
            caughtMissing = true;
        }
        TEST_ASSERT(caughtMissing, "Test J: Failed to raise exception on undefined reference labels");
    }

    std::cout << "All 10 Assembler Unit Tests Passed Successfully!\n";
}