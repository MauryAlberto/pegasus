#pragma once
#include <vector>
#include <cstddef>
#include <variant>
#include "value.hpp"

namespace pegasus {
    struct UpvalueIndex {
        std::size_t index;
    };

    struct ObjUpValue {
        Value* location;
        Value closed{std::monostate{}};
        UpvalueIndex next{SIZE_MAX}; // SIZE_MAX means no next
    };

    struct ObjClosure {
        FunctionIndex funcIndex;
        std::vector<UpvalueIndex> upvalues;
    };
}
