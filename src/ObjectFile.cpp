// src/ObjectFile.cpp
#include "../include/ObjectFile.hpp"
#include <fstream>
#include <cstring>
#include <stdexcept>

// Serializes the ObjectFile structure into a flat big-endian binary stream on disk
void writeObjectFile(const ObjectFile& obj, const std::string& path) {
    std::ofstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot open object file for writing: " + path);

    auto writeU8  = [&](uint8_t  v) { f.write((char*)&v, 1); };
    auto writeU16 = [&](uint16_t v) {
        // Enforce big-endian formatting matching our ISA byte ordering
        uint8_t bytes[2] = {(uint8_t)(v >> 8), (uint8_t)(v & 0xFF)};
        f.write((char*)bytes, 2);
    };
    auto writeU32 = [&](uint32_t v) {
        uint8_t bytes[4] = {
            (uint8_t)(v >> 24), (uint8_t)(v >> 16),
            (uint8_t)(v >>  8), (uint8_t)(v & 0xFF)
        };
        f.write((char*)bytes, 4);
    };
    auto writeStr = [&](const std::string& s) {
        char buf[32] = {}; // Enforce fixed 32-byte constraint for label boundaries
        strncpy(buf, s.c_str(), 31);
        f.write(buf, 32);
    };

    // Header
    writeU32(OBJ_MAGIC);
    writeU16(OBJ_VERSION);
    writeU16((uint16_t)obj.code.size());
    writeU16((uint16_t)obj.symbols.size());
    writeU16((uint16_t)obj.relocations.size());

    // Code Section
    for (uint16_t word : obj.code) writeU16(word);

    // Symbol Table Block
    for (auto& sym : obj.symbols) {
        writeStr(sym.name);
        writeU16(sym.address);
    }

    // Relocation Table Block
    for (auto& rel : obj.relocations) {
        writeU16(rel.codeOffset);
        writeU8(rel.type);
        writeStr(rel.symbolName);
    }
}

// Deserializes a binary object file from disk back into runtime struct vectors
ObjectFile readObjectFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot open object file for reading: " + path);

    auto readU8  = [&]() -> uint8_t  { uint8_t  v; f.read((char*)&v, 1); return v; };
    auto readU16 = [&]() -> uint16_t {
        uint8_t b[2];
        f.read((char*)b, 2);
        return ((uint16_t)b[0] << 8) | b[1];
    };
    auto readU32 = [&]() -> uint32_t {
        uint8_t b[4];
        f.read((char*)b, 4);
        return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) |
               ((uint32_t)b[2] <<  8) | b[3];
    };
    auto readStr = [&]() -> std::string {
        char buf[32];
        f.read(buf, 32);
        return std::string(buf);
    };

    // Layout validation constraints check
    uint32_t magic = readU32();
    if (magic != OBJ_MAGIC)
        throw std::runtime_error("Not a valid RISC object file layout structure: " + path);

    uint16_t version   = readU16();
    if (version != OBJ_VERSION)
        throw std::runtime_error("Unsupported object file format version version error.");

    uint16_t nCode     = readU16();
    uint16_t nSymbols  = readU16();
    uint16_t nRelocs   = readU16();

    ObjectFile obj;
    for (int i = 0; i < nCode;    i++) obj.code.push_back(readU16());
    for (int i = 0; i < nSymbols; i++) {
        auto name = readStr();
        auto addr = readU16();
        obj.symbols.push_back({name, addr});
    }
    for (int i = 0; i < nRelocs;  i++) {
        auto offset = readU16();
        auto type   = readU8();
        auto name   = readStr();
        obj.relocations.push_back({offset, type, name});
    }
    return obj;
}