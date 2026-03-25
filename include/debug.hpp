#pragma once
#include <string>
#include <cstdio>
#include "chunk.hpp"

namespace pegasus {
    inline constexpr bool DEBUG_TRACE_EXECUTION = false;
    void disassembleChunk(const Chunk& chunk, const std::string& name);
    std::size_t disassembleInstruction(const Chunk& chunk, std::size_t offset);
}
