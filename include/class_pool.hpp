#pragma once
#include <vector>
#include <utility>
#include "class_obj.hpp"

namespace pegasus {
    class ClassPool {
        public:
            ClassPool() { classPool_.reserve(32); }
            ClassPool(const ClassPool&) = delete;
            ClassPool& operator=(const ClassPool&) = delete;
            ClassPool(ClassPool&&) = delete;
            ClassPool& operator=(ClassPool&&) = delete;

            ClassIndex addClass(ObjClass&& cls) {
                ClassIndex index{classPool_.size()};
                classPool_.push_back(std::move(cls));
                return index;
            }

            ObjClass& getClass(ClassIndex clsIndex) { return classPool_.at(clsIndex.index); }
            const ObjClass& getClass(ClassIndex clsIndex) const { return classPool_.at(clsIndex.index); }

        private:
            std::vector<ObjClass> classPool_;
    };

    class InstancePool {
        public:
            InstancePool() { instancePool_.reserve(64); }
            InstancePool(const InstancePool&) = delete;
            InstancePool& operator=(const InstancePool&) = delete;
            InstancePool(InstancePool&&) = delete;
            InstancePool& operator=(InstancePool&&) = delete;

            InstanceIndex addInstance(ObjInstance&& inst) {
                InstanceIndex index{instancePool_.size()};
                instancePool_.push_back(std::move(inst));
                return index;
            }

            ObjInstance& getInstance(InstanceIndex instIndex) { return instancePool_.at(instIndex.index); }
            const ObjInstance& getInstance(InstanceIndex instIndex) const { return instancePool_.at(instIndex.index); }

        private:
            std::vector<ObjInstance> instancePool_;
    };
}