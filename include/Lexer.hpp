#pragma once
#include <string>
#include <vector>

enum class Opcode {
    ADA, ADZ, ADC, AWC, ACA, ACZ, ACC, ACW,
    NDU, NDZ, NDC, NCU, NCZ, NCC,
    ADI, LLI, LW, SW, LM, SM, BEQ, BLT, BLE, JAL, JLR, JRI,
    HALT,
    NONE
};

enum class TokenType {
    OPCODE,
    REGISTER,
    IMMEDIATE,
    IDENTIFIER,
    COLON,
    COMMA,
    END_OF_FILE
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int intValue = 0;
    int regNum = -1;
    Opcode op = Opcode::NONE;
};

class Lexer {
public:
    explicit Lexer(std::string source);
    std::vector<Token> scanTokens();

private:
    std::string source;
    std::vector<Token> tokens;
    size_t start = 0;
    size_t current = 0;
    int line = 1;

    bool isAtEnd() const;
    char advance();
    char peek() const;
    char peekNext() const;
    void scanToken();
    void handleWord();
    void handleNumber();
};
