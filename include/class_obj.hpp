#pragma once
#include <string>
#include <unordered_map>
#include "value.hpp"

namespace pegasus {
    struct ObjClass {
        std::string name;
    };

    struct ObjInstance {
        ClassIndex classIndex;
        std::unordered_map<std::string, Value> fields;
    };
}