#pragma once
#include <vector>
#include <utility>
#include "class_obj.hpp"

namespace pegasus {
    class ClassPool {
        public:
            ClassPool() { pool_.reserve(32); }
            ClassPool(const ClassPool&) = delete;
            ClassPool& operator=(const ClassPool&) = delete;
            ClassPool(ClassPool&&) = delete;
            ClassPool& operator=(ClassPool&&) = delete;

            ClassIndex addClass(ObjClass&& cls) {
                ClassIndex index{pool_.size()};
                pool_.push_back(std::move(cls));
                return index;
            }

            ObjClass& getClass(ClassIndex clsIndex) { return pool_.at(clsIndex.index); }
            const ObjClass& getClass(ClassIndex clsIndex) const { return pool_.at(clsIndex.index); }

        private:
            std::vector<ObjClass> pool_;
    };

    class InstancePool {
        public:
            InstancePool() { pool_.reserve(64); }
            InstancePool(const InstancePool&) = delete;
            InstancePool& operator=(const InstancePool&) = delete;
            InstancePool(InstancePool&&) = delete;
            InstancePool& operator=(InstancePool&&) = delete;

            InstanceIndex addInstance(ObjInstance&& inst) {
                InstanceIndex index{pool_.size()};
                pool_.push_back(std::move(inst));
                return index;
            }

            ObjInstance& getInstance(InstanceIndex instIndex) {
                return pool_.at(instIndex.index);
            }

            const ObjInstance& getInstance(InstanceIndex instIndex) const {
                return pool_.at(instIndex.index);
            }

        private:
            std::vector<ObjInstance> pool_;
    };

    class BoundMethodPool {
        public:
            BoundMethodPool() { pool_.reserve(64); }
            BoundMethodPool(const BoundMethodPool&) = delete;
            BoundMethodPool& operator=(const BoundMethodPool&) = delete;
            BoundMethodPool(BoundMethodPool&&) = delete;
            BoundMethodPool operator=(BoundMethodPool&&) = delete;

            BoundMethodIndex addBoundMethod(ObjBoundMethod&& bm) {
                BoundMethodIndex index{pool_.size()};
                pool_.push_back(std::move(bm));
                return index;
            }

            ObjBoundMethod& getBoundMethod(BoundMethodIndex bmIndex) {
                return pool_.at(bmIndex.index);
            }

            const ObjBoundMethod& getBoundMethod(BoundMethodIndex bmIndex) const {
                return pool_.at(bmIndex.index);
            }

        private:
            std::vector<ObjBoundMethod> pool_;
    };
}