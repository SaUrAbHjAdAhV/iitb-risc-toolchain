cmake_minimum_required(VERSION 3.15)
project(IITBRISCToolchain VERSION 1.0 LANGUAGES CXX)

# Enforce C++20 instead of letting the compiler guess
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(include)

add_executable(risc_toolchain 
    src/main.cpp 
    src/Lexer.cpp
)
