#pragma once
#include "closure.hpp"

namespace pegasus {
    class ClosurePool {
        public:
            ClosurePool() { closurePool_.reserve(64); }
        private:
            std::vector<ObjClosure> closurePool_;
    };
}