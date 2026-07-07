#include "../include/Lexer.hpp"
#include <iostream>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <unordered_map>

static const std::unordered_map<std::string, Opcode> OPCODES = {
    {"ada", Opcode::ADA}, {"adz", Opcode::ADZ}, {"adc", Opcode::ADC}, {"awc", Opcode::AWC},
    {"aca", Opcode::ACA}, {"acz", Opcode::ACZ}, {"acc", Opcode::ACC}, {"acw", Opcode::ACW},
    {"ndu", Opcode::NDU}, {"ndz", Opcode::NDZ}, {"ndc", Opcode::NDC}, {"ncu", Opcode::NCU},
    {"ncz", Opcode::NCZ}, {"ncc", Opcode::NCC},
    {"adi", Opcode::ADI}, {"lli", Opcode::LLI}, {"lw",  Opcode::LW},  {"sw",  Opcode::SW},
    {"lm",  Opcode::LM},  {"sm",  Opcode::SM},  {"beq", Opcode::BEQ}, {"blt", Opcode::BLT},
    {"ble", Opcode::BLE}, {"jal", Opcode::JAL}, {"jlr", Opcode::JLR}, {"jri", Opcode::JRI},
    {"halt", Opcode::HALT}
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
            while (peek() != '\n' && !isAtEnd()) advance();
            break;
        case '-':
            if (std::isdigit(peek())) {
                handleNumber();
            } else {
                throw std::runtime_error(
                    "Lexer Error: Stray '-' without digits on line " + std::to_string(line));
            }
            break;
        default:
            if (std::isdigit(c)) {
                handleNumber();
            } else if (std::isalpha(c) || c == '_') {
                handleWord();
            } else {
                throw std::runtime_error(
                    "Lexer Error: Unexpected character '" + std::string(1, c)
                    + "' on line " + std::to_string(line));
            }
            break;
    }
}

void Lexer::handleWord() {
    while (std::isalnum(peek()) || peek() == '_') advance();

    std::string lexeme = source.substr(start, current - start);
    std::string lower = lexeme;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    auto opIt = OPCODES.find(lower);
    if (opIt != OPCODES.end()) {
        tokens.push_back(Token{TokenType::OPCODE, lexeme, line, 0, -1, opIt->second});
        return;
    }

    if (lower.length() == 2 && lower[0] == 'r' && lower[1] >= '0' && lower[1] <= '7') {
        int regNum = lower[1] - '0';
        tokens.push_back(Token{TokenType::REGISTER, lexeme, line, 0, regNum, Opcode::NONE});
        return;
    }

    tokens.push_back(Token{TokenType::IDENTIFIER, lexeme, line, 0, -1, Opcode::NONE});
}

void Lexer::handleNumber() {
    bool isNegative = (source[start] == '-');
    bool isHex = false;

    // Find where the digits actually start, accounting for an optional leading '-'
    // peek()/advance() are already positioned correctly since scanToken() called
    // advance() to consume the first char before dispatching here.
    // For the '-' case: start points at '-', current is one past it.
    // For the digit case: start points at the first digit, current is one past it.

    // Check for hex prefix: either we're at '0' now (digit case, source[start]=='0')
    // or we just consumed '-' and the next two chars are '0x' (negative hex case).
    // FIXED:
    char firstDigit = isNegative ? peek() : source[start];

    // When NOT negative: source[start] is the first digit (already consumed),
    // so the NEXT character is peek(), not peekNext().
    // When negative: peek() is '0' and peekNext() is 'x'/'X'.
    bool hexPrefix = (firstDigit == '0') &&
                    (isNegative ? (peekNext() == 'x' || peekNext() == 'X')
                                : (peek()     == 'x' || peek()     == 'X'));

    if (hexPrefix) {
        isHex = true;
        if (isNegative) {
            // State: start='-', current is one past '-'
            // peek() is '0', peekNext() is 'x'
            advance(); // consume '0'
            advance(); // consume 'x'
        } else {
            // State: start='0', current is one past '0' (scanToken pre-consumed it)
            // peek() is 'x'
            advance(); // consume 'x' — that's all, '0' is already behind us
        }
        // Now current is positioned right after 'x'/'X' in both cases
        if (!std::isxdigit(peek())) {
            throw std::runtime_error(
                "Lexer Error: '0x' prefix with no hex digits on line " + std::to_string(line));
        }
        while (std::isxdigit(peek())) advance();
    } else {
        // Decimal — just consume remaining digits
        // (the first digit or '-' was already consumed by advance() in scanToken)
        while (std::isdigit(peek())) advance();
    }

    std::string lexeme = source.substr(start, current - start);
    int value = 0;
    try {
        // stoi handles the leading '-' sign correctly for both bases
        value = std::stoi(lexeme, nullptr, isHex ? 16 : 10);
    } catch (const std::exception& e) {
        throw std::runtime_error(
            "Lexer Error: Malformed numeric constant '" + lexeme
            + "' on line " + std::to_string(line));
    }

    tokens.push_back(Token{TokenType::IMMEDIATE, lexeme, line, value, -1, Opcode::NONE});
}