#pragma once
#include <cstdint>
#include <string>
#include "chunk.hpp"

namespace pegasus {
    struct ObjFunction {
        std::uint8_t arity{0};
        std::uint8_t upvalueCount{0};
        std::string name{""};
        Chunk chunk{};
    };
}