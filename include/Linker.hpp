// include/Linker.hpp
#pragma once
#include "ObjectFile.hpp"
#include <vector>
#include <string>
#include <unordered_map>

class Linker {
public:
    // Registers an object file module into the linker's aggregation pool
    void addObject(const ObjectFile& obj);

    // Runs the multi-pass compilation loop to resolve symbols and emit the final machine words
    std::vector<uint16_t> link(); 

private:
    std::vector<ObjectFile> objects; // Collection of registered modules 
    // Internal data tables built dynamically during the link execution pass:
    std::unordered_map<std::string, uint16_t> globalSymbolTable; // Maps label names to final global byte addresses 
    std::vector<uint16_t> mergedCode;                           // Stitched execution block 
};