#pragma once
#include <vector>
#include "closure.hpp"

namespace pegasus {
    class UpvaluePool {
        public:
            UpvaluePool() { upvaluePool_.reserve(64); }

            UpvalueIndex addUpValue(ObjUpValue&& uv) {
                UpvalueIndex index{upvaluePool_.size()};
                upvaluePool_.push_back(std::move(uv));
                return index;
            }

            ObjUpValue& getUpvalue(UpvalueIndex uvIndex) {
                return upvaluePool_.at(uvIndex.index);
            }
        private:
            std::vector<ObjUpValue> upvaluePool_;
    };
}