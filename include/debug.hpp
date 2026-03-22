#pragma once
#include <string>
#include <cstdio>
#include <type_traits>
#include "chunk.hpp"

namespace pegasus {
    void disassembleChunk(const Chunk& chunk, const std::string& name);
    std::size_t disassembleInstruction(const Chunk& chunk, std::size_t offset);
}
