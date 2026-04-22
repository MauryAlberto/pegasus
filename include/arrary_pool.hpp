#pragma once
#include <vector>
#include <utility>
#include "array_obj.hpp"
#include "array_index.hpp"

namespace pegasus {
    class ArrayPool {
        public:
            ArrayPool() { pool_.reserve(64); }
            ArrayPool(const ArrayPool&) = delete;
            ArrayPool& operator=(const ArrayPool&) = delete;
            ArrayPool(ArrayPool&&) = delete;
            ArrayPool& operator=(ArrayPool&&) = delete;

            ArrayIndex addArray(ObjArray&& arr) {
                ArrayIndex index{pool_.size()};
                pool_.push_back(std::move(arr));
                return index;
            }

            ObjArray& getArray(ArrayIndex arrIndex) {
                return pool_.at(arrIndex.index);
            }

            const ObjArray& getArray(ArrayIndex arrIndex) const {
                return pool_.at(arrIndex.index);
            }
            
        private:
        std::vector<ObjArray> pool_;
    };
}