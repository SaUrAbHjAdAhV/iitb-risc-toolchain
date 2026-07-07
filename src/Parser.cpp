#include "../include/Parser.hpp"
#include <stdexcept>

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens) {}

Token Parser::peek() const {
    if (isAtEnd()) return tokens.back(); // Returns the trailing EOF token
    return tokens[current];
}

Token Parser::peekNext() const {
    if (current + 1 >= tokens.size()) return tokens.back();
    return tokens[current + 1];
}

Token Parser::advance() {
    if (!isAtEnd()) current++;
    return tokens[current - 1];
}

bool Parser::isAtEnd() const {
    return current >= tokens.size() || tokens[current].type == TokenType::END_OF_FILE;
}

Token Parser::consume(TokenType expected, const std::string& errorMsg) {
    if (peek().type == expected) return advance();
    throw std::runtime_error("Line " + std::to_string(peek().line) + ": " + errorMsg);
}

InstrFormat Parser::formatFor(Opcode op) {
    // TODO: Write a switch statement mapping each of your 26 opcodes
    // to their corresponding format (InstrFormat::R, I, or J) based on your isa_spec.md
    switch(op){
        case Opcode::ADA :
        case Opcode::ACA :
        case Opcode::ACC :
        case Opcode::ACW :
        case Opcode::ACZ :
        case Opcode::ADC :
        case Opcode::ADZ :
        case Opcode::AWC :
        case Opcode::NCC :
        case Opcode::NCU :
        case Opcode::NCZ :
        case Opcode::NDC :
        case Opcode::NDU :
        case Opcode::NDZ :
            return InstrFormat::R;
        case Opcode::ADI :
        case Opcode::LW :
        case Opcode::SW :
        case Opcode::BEQ :
        case Opcode::BLE :
        case Opcode::BLT :
        case Opcode::JLR :
            return InstrFormat::I;
        case Opcode::LLI :
        case Opcode::LM :
        case Opcode::SM :
        case Opcode::JAL :
        case Opcode::JRI :
            return InstrFormat::J;
        default :
            return InstrFormat::UNKNOWN;
    }
    return InstrFormat::UNKNOWN;
}

std::vector<ParsedLine> Parser::parse() {
    std::vector<ParsedLine> results;
    while (!isAtEnd()) {
        ParsedLine line = parseLine();
        // Keep lines that either declare a label or carry an execution opcode
        if (line.label.has_value() || line.hasInstruction) {
            results.push_back(line);
        }
    }
    return results;
}

ParsedLine Parser::parseLine() {
    ParsedLine lineData;
    lineData.line = peek().line;

    // 1. Detect Label Declarations
    if (peek().type == TokenType::IDENTIFIER && peekNext().type == TokenType::COLON) {
        lineData.label = peek().lexeme;
        advance(); // Consume IDENTIFIER
        advance(); // Consume COLON
    }

    // 2. Detect Instruction Definitions
    if (peek().type == TokenType::OPCODE) {
        Token opToken = advance();
        lineData.hasInstruction = true;
        lineData.op = opToken.op;
        lineData.format = formatFor(opToken.op);

        if (lineData.op == Opcode::HALT) {
            return lineData; // No operands to parse, return early!
        }   

        // TODO: Construct operand parsing rules using your grammar switch
        switch (lineData.format) {
            case InstrFormat::R: {
                // Expect: REGISTER, COMMA, REGISTER, COMMA, REGISTER
                // Example: consume(TokenType::REGISTER, "Message")
                // 1. Consume and extract Register A (Destination)
                Token rA = consume(TokenType::REGISTER, "Expected destination register (R0-R7) for R-type instruction.");
                Operand opA;
                opA.kind = Operand::Kind::REGISTER;
                opA.regNum = rA.regNum;
                lineData.operands.push_back(opA);

                // 2. Consume the first separating comma
                consume(TokenType::COMMA, "Expected ',' separating the first and second operands.");

                // 3. Consume and extract Register B (Source 1)
                Token rB = consume(TokenType::REGISTER, "Expected second register operand (R0-R7) for R-type instruction.");
                Operand opB;
                opB.kind = Operand::Kind::REGISTER;
                opB.regNum = rB.regNum;
                lineData.operands.push_back(opB);

                // 4. Consume the second separating comma
                consume(TokenType::COMMA, "Expected ',' separating the second and third operands.");

                // 5. Consume and extract Register C (Source 2)
                Token rC = consume(TokenType::REGISTER, "Expected third register operand (R0-R7) for R-type instruction.");
                Operand opC;
                opC.kind = Operand::Kind::REGISTER;
                opC.regNum = rC.regNum;
                lineData.operands.push_back(opC);

                break;
            }
            case InstrFormat::I: {
                // Split logic dynamically or write single custom parsing case flows:
                // Standard I-type (LW, SW, BEQ, BLT, BLE) -> REG, COMMA, REG, COMMA, (IMM/IDENTIFIER)
                // ADI Quirky -> REG, COMMA, REG, COMMA, IMM
                // JLR Quirky -> REG, COMMA, REG

                //1. Register A is common
                Token rA = consume(TokenType::REGISTER, "Expected register operand (R0-R7) for J-type instruction.");
                Operand opA;
                opA.kind = Operand::Kind::REGISTER;
                opA.regNum = rA.regNum;
                lineData.operands.push_back(opA);

                // 2. Consume the separating comma
                consume(TokenType::COMMA, "Expected ',' separating register and immediate/label fields.");

                // 3. Consume and extract Register B 
                Token rB = consume(TokenType::REGISTER, "Expected second register operand (R0-R7) for R-type instruction.");
                Operand opB;
                opB.kind = Operand::Kind::REGISTER;
                opB.regNum = rB.regNum;
                lineData.operands.push_back(opB);

                // 4. Separate logic based on the instruction type
                if (lineData.op == Opcode::ADI){
                    consume(TokenType::COMMA, "Expected ',' separating register and immediate/label fields.");

                    Token immToken = consume(TokenType::IMMEDIATE, "Expected an IMM for ADI instruction.");
                    Operand opImm;
                    opImm.kind = Operand::Kind::IMMEDIATE;
                    opImm.immValue = immToken.intValue;
                    lineData.operands.push_back(opImm);
                } else if(lineData.op == Opcode::LW || lineData.op == Opcode::SW || lineData.op == Opcode::BEQ || lineData.op == Opcode::BLE || lineData.op == Opcode::BLT){
                    consume(TokenType::COMMA, "Expected ',' separating register and immediate/label fields.");

                    if (peek().type == TokenType::IMMEDIATE) {
                        Token immToken = advance();
                        Operand opImm;
                        opImm.kind = Operand::Kind::IMMEDIATE;
                        opImm.immValue = immToken.intValue;
                        lineData.operands.push_back(opImm);
                    } else if (peek().type == TokenType::IDENTIFIER) {
                        Token labelToken = advance();
                        Operand opLabel;
                        opLabel.kind = Operand::Kind::LABEL_REF;
                        opLabel.labelName = labelToken.lexeme;
                        lineData.operands.push_back(opLabel);
                    } else {
                        throw std::runtime_error("Line " + std::to_string(peek().line) + 
                            ": Expected numeric immediate value or label reference string after register.");
                    }
                } else if(lineData.op == Opcode::JLR){
                    // JLR expects absolutely nothing after its second register.
                    // If someone wrote a comma, they tried to add a third operand.
                    if (peek().type == TokenType::COMMA) {
                        throw std::runtime_error("Line " + std::to_string(peek().line) + 
                            ": JLR instruction expects exactly 2 register operands.");
                    }
                }
                break;
            }
            case InstrFormat::J: {
                // Standard J-type (LLI, JAL, JRI) -> REG, COMMA, (IMM/IDENTIFIER)
                // LM/SM Vector Block -> REG, COMMA, IMM (8-bit mask field)
                // 1. All J-type instructions start with a register operand (e.g., LLI R1, ...)
                Token rA = consume(TokenType::REGISTER, "Expected register operand (R0-R7) for J-type instruction.");
                Operand opA;
                opA.kind = Operand::Kind::REGISTER;
                opA.regNum = rA.regNum;
                lineData.operands.push_back(opA);

                // 2. Consume the separating comma
                consume(TokenType::COMMA, "Expected ',' separating register and immediate/label fields.");

                // 3. Separate logic based on the instruction type
                if (lineData.op == Opcode::LM || lineData.op == Opcode::SM) {
                    // Vector instructions ONLY accept a numeric mask constant
                    Token immToken = consume(TokenType::IMMEDIATE, "Expected an 8-bit numeric mask value for vector load/store operations.");
                    Operand opImm;
                    opImm.kind = Operand::Kind::IMMEDIATE;
                    opImm.immValue = immToken.intValue;
                    lineData.operands.push_back(opImm);
                } else {
                    // Standard J-types accept either a raw number constant OR a textual label string reference
                    if (peek().type == TokenType::IMMEDIATE) {
                        Token immToken = advance();
                        Operand opImm;
                        opImm.kind = Operand::Kind::IMMEDIATE;
                        opImm.immValue = immToken.intValue;
                        lineData.operands.push_back(opImm);
                    } else if (peek().type == TokenType::IDENTIFIER) {
                        Token labelToken = advance();
                        Operand opLabel;
                        opLabel.kind = Operand::Kind::LABEL_REF;
                        opLabel.labelName = labelToken.lexeme;
                        lineData.operands.push_back(opLabel);
                    } else {
                        throw std::runtime_error("Line " + std::to_string(peek().line) + 
                            ": Expected numeric immediate value or label reference string after register.");
                    }
                }
                break;
            }
            default:
                throw std::runtime_error("Line " + std::to_string(lineData.line) + ": Unknown instruction shape format.");
        }
    } else if (lineData.label.has_value()) {
        // Line simply had a label declaration alone. This is completely legal.
    } else {
        // Encountered parsing garbage or unexpected structural punctuation
        throw std::runtime_error("Line " + std::to_string(peek().line) + ": Assembly syntax error, unexpected token '" + peek().lexeme + "'");
    }

    return lineData;
}