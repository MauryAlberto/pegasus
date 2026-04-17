#pragma once
#include <string>
#include <unordered_map>
#include "value.hpp"
#include "closure_index.hpp"
#include "instance_index.hpp"

namespace pegasus {
    struct ObjClass {
        std::string name;
        std::unordered_map<std::string, ClosureIndex> methods;
    };

    struct ObjInstance {
        ClassIndex classIndex;
        std::unordered_map<std::string, Value> fields;
    };

    struct ObjBoundMethod {
        InstanceIndex receiver;
        ClosureIndex method;
    };
}