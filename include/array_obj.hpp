#pragma once
#include <vector>
#include "value.hpp"

namespace pegasus {
    struct ObjArray {
        std::vector<Value> elements;
    };
}