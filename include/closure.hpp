#pragma once
#include <vector>
#include <cstddef>
#include "value.hpp"

namespace pegasus {
    struct ClosreIndex {
        std::size_t index_;
    };

    struct UpValue {
        Value* location_;
    };

    struct ObjClosure {
        ObjClosure() { upValues_.reserve(8); }
        FunctionIndex funcIndex_;
        std::vector<UpValue> upValues_;
    };
}
