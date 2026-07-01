#include "../include/Lexer.hpp"
#include <iostream>
#include <algorithm>
#include <cctype>
#include <unordered_map>

// A static map containing all 26 distinct opcodes from your ISA spec
static const std::unordered_map<std::string, Opcode> OPCODES = {
    {"ada", Opcode::ADA}, {"adz", Opcode::ADZ}, {"adc", Opcode::ADC}, {"awc", Opcode::AWC},
    {"aca", Opcode::ACA}, {"acz", Opcode::ACZ}, {"acc", Opcode::ACC}, {"acw", Opcode::ACW},
    {"ndu", Opcode::NDU}, {"ndz", Opcode::NDZ}, {"ndc", Opcode::NDC}, {"ncu", Opcode::NCU},
    {"ncz", Opcode::NCZ}, {"ncc", Opcode::NCC},
    {"adi", Opcode::ADI}, {"lli", Opcode::LLI}, {"lw",  Opcode::LW},  {"sw",  Opcode::SW},
    {"lm",  Opcode::LM},  {"sm",  Opcode::SM},  {"beq", Opcode::BEQ}, {"blt", Opcode::BLT},
    {"ble", Opcode::BLE}, {"jal", Opcode::JAL}, {"jlr", Opcode::JLR}, {"jri", Opcode::JRI}
};

Lexer::Lexer(std::string source) : source(source) {}

bool Lexer::isAtEnd() const {
    return current >= source.length();
}

char Lexer::advance() {
    return source[current++];
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source[current];
}

char Lexer::peekNext() const {
    if (current + 1 >= source.length()) return '\0';
    return source[current + 1];
}

std::vector<Token> Lexer::scanTokens() {
    while (!isAtEnd()) {
        start = current;
        scanToken();
    }
    tokens.push_back(Token{TokenType::END_OF_FILE, "", line});
    return tokens;
}

void Lexer::scanToken() {
    char c = advance();
    switch (c) {
        case ' ':
        case '\r':
        case '\t':
            // Ignore normal whitespace
            break;
        case '\n':
            line++;
            break;
        case ',':
            tokens.push_back(Token{TokenType::COMMA, ",", line});
            break;
        case ':':
            tokens.push_back(Token{TokenType::COLON, ":", line});
            break;
        case ';':
            // Comment leader: consume until the end of the line
            while (peek() != '\n' && !isAtEnd()) {
                advance();
            }
            break;
        case '-':
            // Negative number check: must be immediately followed by a digit
            if (std::isdigit(peek())) {
                handleNumber();
            } else {
                std::cerr << "Lexer Error: Stray '-' without digits on line " << line << "\n";
                exit(1);
            }
            break;
        default:
            if (std::isdigit(c)) {
                handleNumber();
            } else if (std::isalpha(c) || c == '_') {
                handleWord();
            } else {
                std::cerr << "Lexer Error: Unexpected character '" << c << "' on line " << line << "\n";
                exit(1);
            }
            break;
    }
}

void Lexer::handleWord() {
    // 1. Consume maximum alphanumeric string (Word Priority / Maximal Munch)
    while (std::isalnum(peek()) || peek() == '_') {
        advance();
    }

    std::string lexeme = source.substr(start, current - start);
    
    // Create a lowercase version for case-insensitive checking (Opcodes & Registers)
    std::string lowerLexeme = lexeme;
    std::transform(lowerLexeme.begin(), lowerLexeme.end(), lowerLexeme.begin(), ::tolower);

    // 2. Check if it matches an Opcode
    auto opIt = OPCODES.find(lowerLexeme);
    if (opIt != OPCODES.end()) {
        tokens.push_back(Token{TokenType::OPCODE, lexeme, line, 0, -1, opIt->second});
        return;
    }

    // 3. Check if it matches a Register pattern (r0-r7)
    if (lowerLexeme.length() == 2 && (lowerLexeme[0] == 'r' || lowerLexeme[0] == 'R') && lowerLexeme[1] >= '0' && lowerLexeme[1] <= '7') {
        int regNum = lowerLexeme[1] - '0';
        tokens.push_back(Token{TokenType::REGISTER, lexeme, line, 0, regNum, Opcode::NONE});
        return;
    }

    // 4. Default fallthrough: It's an identifier (a label name or reference)
    tokens.push_back(Token{TokenType::IDENTIFIER, lexeme, line, 0, -1, Opcode::NONE});
}

void Lexer::handleNumber() {
    bool isHex = false;
    
    // Check for Hexadecimal prefix (0x or 0X)
    if (source[start] == '0' && (peek() == 'x' || peek() == 'X')) {
        advance(); // consume 'x'
        isHex = true;
        
        // Consume valid hex digits
        while (std::isxdigit(peek())) {
            advance();
        }
    } else {
        // Consume normal decimal digits
        while (std::isdigit(peek())) {
            advance();
        }
    }

    std::string lexeme = source.substr(start, current - start);
    int value = 0;

    try {
        if (isHex) {
            value = std::stoi(lexeme, nullptr, 16);
        } else {
            value = std::stoi(lexeme, nullptr, 10);
        }
    } catch (...) {
        std::cerr << "Lexer Error: Malformed number numeric constant '" << lexeme << "' on line " << line << "\n";
        exit(1);
    }

    tokens.push_back(Token{TokenType::IMMEDIATE, lexeme, line, value, -1, Opcode::NONE});
}