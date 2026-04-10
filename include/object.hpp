#pragma once
#include <cstdint>
#include <string>
#include "chunk.hpp"

namespace pegasus {
    struct ObjFunction {
        std::uint8_t arity_{0};
        std::string name_{""};
        Chunk chunk_{};
    };
}