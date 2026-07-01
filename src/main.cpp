#include "../include/Lexer.hpp"
#include <iostream>
#include <cassert>

// A quick helper to print out what token types we are testing for visual checks
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

int main() {
    std::cout << "===========================================\n";
    std::cout << "Starting Day 2 Lexer Verification Tests...\n";
    std::cout << "===========================================\n\n";

    // Test Case 1: Standard uppercase instruction
    {
        std::cout << "Running Test 1: Standard R-type Instruction...\n";
        Lexer lexer("ADA R1, R2, R3");
        auto tokens = lexer.scanTokens();
        
        assert(tokens.size() == 7); // ADA, R1, COMMA, R2, COMMA, R3, EOF
        assert(tokens[0].type == TokenType::OPCODE);
        assert(tokens[0].op == Opcode::ADA);
        assert(tokens[1].type == TokenType::REGISTER && tokens[1].regNum == 1);
        assert(tokens[2].type == TokenType::COMMA);
        assert(tokens[3].type == TokenType::REGISTER && tokens[3].regNum == 2);
        assert(tokens[4].type == TokenType::COMMA);
        assert(tokens[5].type == TokenType::REGISTER && tokens[5].regNum == 3);
    }

    // Test Case 2: Label, Immediate, and whitespace management
    {
        std::cout << "Running Test 2: Label definition and signed immediate...\n";
        Lexer lexer("loop: ADI R1, R0, -4");
        auto tokens = lexer.scanTokens();

        assert(tokens[0].type == TokenType::IDENTIFIER && tokens[0].lexeme == "loop");
        assert(tokens[1].type == TokenType::COLON);
        assert(tokens[2].type == TokenType::OPCODE && tokens[2].op == Opcode::ADI);
        assert(tokens[3].type == TokenType::REGISTER && tokens[3].regNum == 1);
        assert(tokens[4].type == TokenType::COMMA);
        assert(tokens[5].type == TokenType::REGISTER && tokens[5].regNum == 0);
        assert(tokens[6].type == TokenType::COMMA);
        assert(tokens[7].type == TokenType::IMMEDIATE && tokens[7].intValue == -4);
    }

    // Test Case 3: Pure comment handling
    {
        std::cout << "Running Test 3: Pure comment lines...\n";
        Lexer lexer("; just a comment");
        auto tokens = lexer.scanTokens();
        assert(tokens.size() == 1); // Should only contain the trailing EOF
        assert(tokens[0].type == TokenType::END_OF_FILE);
    }

    // Test Case 4: Trailing comments on instructions
    {
        std::cout << "Running Test 4: Trailing comments...\n";
        Lexer lexer("ADA R1, R2, R3 ; result");
        auto tokens = lexer.scanTokens();
        assert(tokens.size() == 7); // Comment completely trimmed away, matches Test 1 size
        assert(tokens[5].type == TokenType::REGISTER && tokens[5].regNum == 3);
    }

    // Test Case 5: Empty line checking and line count alignment
    {
        std::cout << "Running Test 5: Empty line structure and line metrics...\n";
        Lexer lexer("ADA R1\n\n\nADI R2");
        auto tokens = lexer.scanTokens();
        // Index mapping: ADA(L1), R1(L1), ADI(L4), R2(L4), EOF(L4)
        assert(tokens[0].line == 1);
        assert(tokens[1].line == 1);
        assert(tokens[2].type == TokenType::OPCODE && tokens[2].line == 4);
        assert(tokens[3].type == TokenType::REGISTER && tokens[3].line == 4);
    }

    // Test Case 6: Case insensitivity and missing spacing overrides
    {
        std::cout << "Running Test 6: Case folding and tight spacing...\n";
        Lexer lexer("ada r1,r2,r3");
        auto tokens = lexer.scanTokens();
        assert(tokens.size() == 7);
        assert(tokens[0].type == TokenType::OPCODE && tokens[0].op == Opcode::ADA);
        assert(tokens[1].type == TokenType::REGISTER && tokens[1].regNum == 1);
    }

    // Test Case 7: Word Priority (Maximal Munch) validation vs Register definitions
    {
        std::cout << "Running Test 7: Word boundary priority over fake registers (R9)...\n";
        Lexer lexer("R9");
        auto tokens = lexer.scanTokens();
        assert(tokens[0].type == TokenType::IDENTIFIER && tokens[0].lexeme == "R9");
    }

    // Test Case 8: Hexadecimal numeric base formatting
    {
        std::cout << "Running Test 8: Hexadecimal base reading overrides (0x2A)...\n";
        Lexer lexer("lli ra, 0x2A");
        auto tokens = lexer.scanTokens();
        assert(tokens[3].type == TokenType::IMMEDIATE);
        assert(tokens[3].intValue == 42); // Hex 0x2A converted safely to int 42
    }

    std::cout << "\n===========================================\n";
    std::cout << "🎉 ALL 8 TEST CASES PASSED SUCCESSFULLY! 🎉\n";
    std::cout << "===========================================\n";
    return 0;
}