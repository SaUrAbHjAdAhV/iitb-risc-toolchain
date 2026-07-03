#pragma once
#include "Lexer.hpp"
#include <string>
#include <vector>
#include <optional>
#include <cstdint>

enum class InstrFormat { R, I, J, UNKNOWN };

struct Operand {
    enum class Kind { REGISTER, IMMEDIATE, LABEL_REF };
    Kind kind;
    int  regNum   = -1;   // for REGISTER
    int  immValue = 0;    // for IMMEDIATE
    std::string labelName; // for LABEL_REF
};

struct ParsedLine {
    std::optional<std::string> label;  // label defined on this line, if any
    bool hasInstruction = false;
    Opcode   op     = Opcode::NONE;
    InstrFormat format = InstrFormat::UNKNOWN;
    std::vector<Operand> operands;
    int line = 0;           // source line number, for error messages
    uint16_t address = 0;   // filled in by Pass 1
};