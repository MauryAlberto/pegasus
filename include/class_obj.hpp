#pragma once
#include <string>
#include <unordered_map>
#include "value.hpp"

namespace pegasus {
    struct ClassIndex {
        std::size_t index;
    };

    struct InstanceIndex {
        std::size_t index;
    };

    struct ObjClass {
        std::string name;
    };

    struct ObjInstance {
        ClassIndex classIndex;
        std::unordered_map<std::string, Value> fields;
    };
}