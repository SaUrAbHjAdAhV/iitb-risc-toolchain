cmake_minimum_required(VERSION 3.10)
project(IITB_RISC_Toolchain CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

# 1. Main Assembler Executable
add_executable(risc_toolchain 
    src/main.cpp 
    src/Lexer.cpp
)

# 2. Test Suite Executable
add_executable(run_tests 
    tests/test_lexer.cpp 
    src/Lexer.cpp
)