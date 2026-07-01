#include "../include/Lexer.hpp"
#include <iostream>
#include <cassert>
#include <stdexcept>
#include <cstdlib>

std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::OPCODE:      return "OPCODE";
        case TokenType::REGISTER:    return "REGISTER";
        case TokenType::IMMEDIATE:   return "IMMEDIATE";
        case TokenType::IDENTIFIER:  return "IDENTIFIER";
        case TokenType::COLON:       return "COLON";
        case TokenType::COMMA:       return "COMMA";
        case TokenType::END_OF_FILE: return "END_OF_FILE";
    }
    return "UNKNOWN";
}

// Helper: assert with a descriptive message so you know exactly which
// assertion failed without staring at a line number
#define CHECK(expr) do { \
    if (!(expr)) { \
        std::cerr << "FAILED: " << #expr << " (line " << __LINE__ << ")\n"; \
        std::exit(1); \
    } \
} while(0)

int main() {
    std::cout << "===========================================\n";
    std::cout << "IITB-RISC-23 Lexer Verification Tests\n";
    std::cout << "===========================================\n\n";

    // -------------------------------------------------------
    // Test 1: Standard R-type — uppercase, spaces after commas
    // -------------------------------------------------------
    {
        std::cout << "Test  1: Standard R-type instruction (ADA R1, R2, R3)...\n";
        Lexer lexer("ADA R1, R2, R3");
        auto tokens = lexer.scanTokens();
        // Expected: OPCODE, REG(1), COMMA, REG(2), COMMA, REG(3), EOF  = 7 tokens
        CHECK(tokens.size() == 7);
        CHECK(tokens[0].type == TokenType::OPCODE && tokens[0].op == Opcode::ADA);
        CHECK(tokens[1].type == TokenType::REGISTER && tokens[1].regNum == 1);
        CHECK(tokens[2].type == TokenType::COMMA);
        CHECK(tokens[3].type == TokenType::REGISTER && tokens[3].regNum == 2);
        CHECK(tokens[4].type == TokenType::COMMA);
        CHECK(tokens[5].type == TokenType::REGISTER && tokens[5].regNum == 3);
        CHECK(tokens[6].type == TokenType::END_OF_FILE);
        std::cout << "  PASSED\n";
    }

    // -------------------------------------------------------
    // Test 2: Label + ADI with negative decimal immediate
    // -------------------------------------------------------
    {
        std::cout << "Test  2: Label + ADI with negative immediate (loop: ADI R1, R0, -4)...\n";
        Lexer lexer("loop: ADI R1, R0, -4");
        auto tokens = lexer.scanTokens();
        // Expected: IDENT(loop), COLON, OPCODE(ADI), REG(1), COMMA, REG(0), COMMA, IMM(-4), EOF = 9
        CHECK(tokens.size() == 9);
        CHECK(tokens[0].type == TokenType::IDENTIFIER && tokens[0].lexeme == "loop");
        CHECK(tokens[1].type == TokenType::COLON);
        CHECK(tokens[2].type == TokenType::OPCODE && tokens[2].op == Opcode::ADI);
        CHECK(tokens[3].type == TokenType::REGISTER && tokens[3].regNum == 1);
        CHECK(tokens[4].type == TokenType::COMMA);
        CHECK(tokens[5].type == TokenType::REGISTER && tokens[5].regNum == 0);
        CHECK(tokens[6].type == TokenType::COMMA);
        CHECK(tokens[7].type == TokenType::IMMEDIATE && tokens[7].intValue == -4);
        CHECK(tokens[8].type == TokenType::END_OF_FILE);
        std::cout << "  PASSED\n";
    }

    // -------------------------------------------------------
    // Test 3: Pure comment line produces only EOF
    // -------------------------------------------------------
    {
        std::cout << "Test  3: Pure comment line (; just a comment)...\n";
        Lexer lexer("; just a comment");
        auto tokens = lexer.scanTokens();
        CHECK(tokens.size() == 1);
        CHECK(tokens[0].type == TokenType::END_OF_FILE);
        std::cout << "  PASSED\n";
    }

    // -------------------------------------------------------
    // Test 4: Trailing comment is fully stripped
    // -------------------------------------------------------
    {
        std::cout << "Test  4: Trailing comment stripped (ADA R1, R2, R3 ; result)...\n";
        Lexer lexer("ADA R1, R2, R3 ; result");
        auto tokens = lexer.scanTokens();
        CHECK(tokens.size() == 7); // Identical to Test 1 — comment contributes nothing
        CHECK(tokens[5].type == TokenType::REGISTER && tokens[5].regNum == 3);
        CHECK(tokens[6].type == TokenType::END_OF_FILE);
        std::cout << "  PASSED\n";
    }

    // -------------------------------------------------------
    // Test 5: Blank lines don't break line counting
    // -------------------------------------------------------
    {
        std::cout << "Test  5: Blank lines preserved in line counter...\n";
        Lexer lexer("ADA R1\n\n\nADI R2");
        auto tokens = lexer.scanTokens();
        // ADA on line 1, R1 on line 1, ADI on line 4, R2 on line 4, EOF
        CHECK(tokens[0].line == 1);
        CHECK(tokens[1].line == 1);
        CHECK(tokens[2].type == TokenType::OPCODE && tokens[2].line == 4);
        CHECK(tokens[3].type == TokenType::REGISTER && tokens[3].line == 4);
        std::cout << "  PASSED\n";
    }

    // -------------------------------------------------------
    // Test 6: Case-insensitive mnemonics and no spaces after commas
    // -------------------------------------------------------
    {
        std::cout << "Test  6: Lowercase + tight spacing (ada r1,r2,r3)...\n";
        Lexer lexer("ada r1,r2,r3");
        auto tokens = lexer.scanTokens();
        CHECK(tokens.size() == 7);
        CHECK(tokens[0].type == TokenType::OPCODE && tokens[0].op == Opcode::ADA);
        CHECK(tokens[1].type == TokenType::REGISTER && tokens[1].regNum == 1);
        CHECK(tokens[2].type == TokenType::COMMA);
        CHECK(tokens[3].type == TokenType::REGISTER && tokens[3].regNum == 2);
        CHECK(tokens[5].type == TokenType::REGISTER && tokens[5].regNum == 3);
        std::cout << "  PASSED\n";
    }

    // -------------------------------------------------------
    // Test 7: R9 → IDENTIFIER (not a register, not an error)
    // -------------------------------------------------------
    {
        std::cout << "Test  7: R9 classified as IDENTIFIER, not REGISTER...\n";
        Lexer lexer("R9");
        auto tokens = lexer.scanTokens();
        CHECK(tokens.size() == 2); // IDENT(R9), EOF
        CHECK(tokens[0].type == TokenType::IDENTIFIER);
        CHECK(tokens[0].lexeme == "R9");
        std::cout << "  PASSED\n";
    }

    // -------------------------------------------------------
    // Test 8: R0 and R7 are valid registers (boundary check)
    // -------------------------------------------------------
    {
        std::cout << "Test  8: R0 and R7 are valid register boundaries...\n";
        Lexer lexer("ADA R0, R7, R0");
        auto tokens = lexer.scanTokens();
        CHECK(tokens.size() == 7);
        CHECK(tokens[1].type == TokenType::REGISTER && tokens[1].regNum == 0);
        CHECK(tokens[3].type == TokenType::REGISTER && tokens[3].regNum == 7);
        CHECK(tokens[5].type == TokenType::REGISTER && tokens[5].regNum == 0);
        std::cout << "  PASSED\n";
    }

    // -------------------------------------------------------
    // Test 9: Positive hex immediate (0x2A = 42)
    // -------------------------------------------------------
    {
        std::cout << "Test  9: Hex immediate 0x2A = 42...\n";
        // LLI R1, 0x2A — J-type, 2 operands (register + immediate)
        Lexer lexer("LLI R1, 0x2A");
        auto tokens = lexer.scanTokens();
        // Expected: OPCODE(LLI), REG(1), COMMA, IMM(42), EOF = 5
        CHECK(tokens.size() == 5);
        CHECK(tokens[0].type == TokenType::OPCODE && tokens[0].op == Opcode::LLI);
        CHECK(tokens[1].type == TokenType::REGISTER && tokens[1].regNum == 1);
        CHECK(tokens[2].type == TokenType::COMMA);
        CHECK(tokens[3].type == TokenType::IMMEDIATE && tokens[3].intValue == 42);
        std::cout << "  PASSED\n";
    }

    // -------------------------------------------------------
    // Test 10: Negative hex immediate (-0x2A = -42)
    // -------------------------------------------------------
    {
        std::cout << "Test 10: Negative hex immediate -0x2A = -42...\n";
        Lexer lexer("ADI R1, R2, -0x2A");
        auto tokens = lexer.scanTokens();
        CHECK(tokens.size() == 7);
        CHECK(tokens[5].type == TokenType::IMMEDIATE && tokens[5].intValue == -42);
        std::cout << "  PASSED\n";
    }

    // -------------------------------------------------------
    // Test 11: Label on its own line, instruction on next
    // (most real programs look like this)
    // -------------------------------------------------------
    {
        std::cout << "Test 11: Label on its own line before instruction...\n";
        Lexer lexer("loop:\nADA R1, R2, R3");
        auto tokens = lexer.scanTokens();
        // IDENT(loop), COLON on line 1; then OPCODE, REG, COMMA, REG, COMMA, REG, EOF on line 2
        CHECK(tokens[0].type == TokenType::IDENTIFIER && tokens[0].lexeme == "loop");
        CHECK(tokens[0].line == 1);
        CHECK(tokens[1].type == TokenType::COLON);
        CHECK(tokens[2].type == TokenType::OPCODE && tokens[2].line == 2);
        CHECK(tokens[2].op == Opcode::ADA);
        std::cout << "  PASSED\n";
    }

    // -------------------------------------------------------
    // Test 12: LM with 8-bit mask as a decimal immediate
    // mask 0b10000001 = 0x81 = 129 decimal → loads R0 and R7
    // -------------------------------------------------------
    {
        std::cout << "Test 12: LM with 8-bit mask immediate (LM R1, 0x81)...\n";
        Lexer lexer("LM R1, 0x81");
        auto tokens = lexer.scanTokens();
        // OPCODE(LM), REG(1), COMMA, IMM(129), EOF = 5
        CHECK(tokens.size() == 5);
        CHECK(tokens[0].type == TokenType::OPCODE && tokens[0].op == Opcode::LM);
        CHECK(tokens[1].type == TokenType::REGISTER && tokens[1].regNum == 1);
        CHECK(tokens[3].type == TokenType::IMMEDIATE && tokens[3].intValue == 0x81);
        std::cout << "  PASSED\n";
    }

    // -------------------------------------------------------
    // Test 13: All 3 formats in a small multi-line program
    // R-type (ADA), I-type (BEQ), J-type (JAL) — exercises
    // that each format scans cleanly across multiple lines
    // -------------------------------------------------------
    {
        std::cout << "Test 13: Multi-line program exercising R, I, and J formats...\n";
        Lexer lexer(
            "ADA R1, R2, R3\n"
            "BEQ R1, R2, 4\n"
            "JAL R7, 10\n"
        );
        auto tokens = lexer.scanTokens();
        // Line 1: ADA R1 , R2 , R3      = 6 tokens
        // Line 2: BEQ R1 , R2 , IMM(4)  = 6 tokens
        // Line 3: JAL R7 , IMM(10)       = 4 tokens
        // EOF                             = 1 token
        // Total: 17
        CHECK(tokens.size() == 17);
        CHECK(tokens[0].op  == Opcode::ADA);
        CHECK(tokens[6].op  == Opcode::BEQ);
        CHECK(tokens[12].op == Opcode::JAL);
        // Spot-check the JAL immediate
        CHECK(tokens[14].type == TokenType::COMMA);
        CHECK(tokens[15].type == TokenType::IMMEDIATE && tokens[15].intValue == 10);
        std::cout << "  PASSED\n";
    }

    // -------------------------------------------------------
    // Test 14: Invalid character triggers a lexer error
    // Your lexer should throw or set an error flag — NOT
    // silently skip '#'. Adjust the check to match however
    // your Lexer signals errors (exception vs error flag).
    // -------------------------------------------------------
    {
        std::cout << "Test 14: Invalid character '#' triggers lexer error...\n";
        bool errorCaught = false;
        try {
            Lexer lexer("ADA R1 # R2");
            auto tokens = lexer.scanTokens();
            // If your lexer uses an error flag instead of throwing:
            // errorCaught = lexer.hadError();
        } catch (const std::exception& e) {
            errorCaught = true;
        }
        CHECK(errorCaught);
        std::cout << "  PASSED\n";
    }

    // -------------------------------------------------------
    // Test 15: Mixed case mnemonics and registers
    // -------------------------------------------------------
    {
        std::cout << "Test 15: Mixed case (aDa R1, r2, R3)...\n";
        Lexer lexer("aDa R1, r2, R3");
        auto tokens = lexer.scanTokens();
        CHECK(tokens.size() == 7);
        CHECK(tokens[0].type == TokenType::OPCODE && tokens[0].op == Opcode::ADA);
        CHECK(tokens[1].type == TokenType::REGISTER && tokens[1].regNum == 1);
        CHECK(tokens[3].type == TokenType::REGISTER && tokens[3].regNum == 2);
        std::cout << "  PASSED\n";
    }

    std::cout << "\n===========================================\n";
    std::cout << "ALL 15 TESTS PASSED\n";
    std::cout << "===========================================\n";
    return 0;
}