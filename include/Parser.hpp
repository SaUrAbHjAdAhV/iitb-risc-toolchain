#pragma once
#include "Lexer.hpp"
#include "Instruction.hpp"
#include <vector>
#include <string>

class Parser {
public:
    Parser(const std::vector<Token>& tokens);
    std::vector<ParsedLine> parse();

private:
    const std::vector<Token>& tokens;
    size_t current = 0;

    // Stream navigation helpers
    Token peek() const;
    Token peekNext() const;
    Token advance();
    bool isAtEnd() const;
    Token consume(TokenType expected, const std::string& errorMsg);

    // Parsing routing pipeline
    ParsedLine parseLine();
    Operand parseOperand();
    InstrFormat formatFor(Opcode op);
};