#pragma once
#include <vector>
#include <utility>
#include "object.hpp"
#include "function_index.hpp"

namespace pegasus {
    class FunctionPool {
        public:
            FunctionPool() { functionPool_.reserve(64); }
            FunctionPool(const FunctionPool&) = delete;
            FunctionPool& operator=(const FunctionPool&) = delete;
            FunctionPool(FunctionPool&&) = delete;
            FunctionPool& operator=(FunctionPool&&) = delete;

            FunctionIndex addFunction(const ObjFunction& objFunc) {
                FunctionIndex index{functionPool_.size()};
                functionPool_.push_back(objFunc);
                return index;
            }

            FunctionIndex addFunction(ObjFunction&& objFunc) {
                FunctionIndex index{functionPool_.size()};
                functionPool_.push_back(std::move(objFunc));
                return index;
            }

            const ObjFunction& getFunction(const FunctionIndex& funcIndex) const { return functionPool_.at(funcIndex.index_);}
        private:
            std::vector<ObjFunction> functionPool_;
    };
}