#pragma once
#include "chunk.hpp"

namespace pegasus {
    struct ObjFunction {
        std::size_t arity_{0};
        std::string name_{""};
        Chunk chunk_{};
    };
}