#pragma once
#include "closure_obj.hpp"

namespace pegasus {
    class ClosurePool {
        public:
            ClosurePool() { pool_.reserve(64); }
            ClosurePool(const ClosurePool&) = delete;
            ClosurePool& operator=(const ClosurePool&) = delete;
            ClosurePool(ClosureIndex&&) = delete;
            ClosurePool& operator=(ClosureIndex&&) = delete;

            ClosureIndex addClosure(ObjClosure&& closure) {
                ClosureIndex index{pool_.size()};
                pool_.push_back(std::move(closure));
                return index;
            }

            ObjClosure& getClosure(const ClosureIndex& closureIndex) {
                return pool_.at(closureIndex.index);
            }

            const ObjClosure& getClosure(const ClosureIndex& closureIndex) const {
                return pool_.at(closureIndex.index);
            }

        private:
            std::vector<ObjClosure> pool_;
    };
}