cmake_minimum_required(VERSION 3.10)
project(IITB_RISC_Toolchain CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

# 1. Main Toolchain Compiler Executable
add_executable(risc_toolchain 
    src/main.cpp 
    src/Lexer.cpp 
    src/Parser.cpp 
    src/Assembler.cpp
    src/Emulator.cpp
    src/ObjectFile.cpp   
    src/Linker.cpp
)

# 2. Test Suite Executable (Added src/Emulator.cpp to avoid missing symbol link errors)
add_executable(run_tests 
    tests/test_lexer.cpp 
    tests/test_assembler.cpp
    src/Lexer.cpp
    src/Parser.cpp
    src/Assembler.cpp
    src/Emulator.cpp
    src/ObjectFile.cpp   
    src/Linker.cpp
)