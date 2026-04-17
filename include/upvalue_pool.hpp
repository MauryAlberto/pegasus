#pragma once
#include <vector>
#include "closure_obj.hpp"

namespace pegasus {
    class UpvaluePool {
        public:
            UpvaluePool() { pool_.reserve(64); }

            UpvalueIndex addUpValue(ObjUpValue&& uv) {
                UpvalueIndex index{pool_.size()};
                pool_.push_back(std::move(uv));
                return index;
            }

            ObjUpValue& getUpvalue(UpvalueIndex uvIndex) {
                return pool_.at(uvIndex.index);
            }
        private:
            std::vector<ObjUpValue> pool_;
    };
}