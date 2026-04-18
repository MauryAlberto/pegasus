#pragma once
#include <string>
#include <cstdio>
#include "chunk.hpp"
#include "function_pool.hpp"

namespace pegasus {
    constexpr bool DEBUG_TRACE_EXECUTION = false;
    void disassembleChunk(const Chunk* chunk, std::string_view name, const FunctionPool* pool = nullptr);
    std::size_t disassembleInstruction(const Chunk* chunk, std::size_t offset, const FunctionPool* pool = nullptr);
}
