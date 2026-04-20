#pragma once
#include <array>
#include <chrono>
#include <stdexcept>
#include <type_traits>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include "chunk.hpp"
#include "debug.hpp"
#include "value.hpp"
#include "compiler.hpp"
#include "object.hpp"
#include "function_pool.hpp"
#include "closure_pool.hpp"
#include "upvalue_pool.hpp"
#include "class_pool.hpp"

namespace pegasus {
    inline constexpr int FRAME_MAX = 64;
    inline constexpr int STACK_SIZE = FRAME_MAX * 256;

    enum class InterpretResult {
        OK,
        COMPILE_ERROR,
        RUNTIME_ERROR
    };

    enum class BinaryOp {
        ADD,
        SUBTRACT,
        MULTIPLY,
        DIVIDE,
        EQUAL,
        GREATER,
        LESS
    };

    class StringPool {
        public:
            std::string_view intern(std::string_view sv) {
                auto it{pool_.find(std::string(sv))};
                if(it == pool_.end()) {
                    it = pool_.insert(std::string(sv)).first;
                }
                return *it;
            }
        private:
            std::unordered_set<std::string> pool_{};
    };

    struct CallFrame {
        ClosureIndex closureIndex;
        const std::uint8_t* ip;
        Value* slots;
    };

    class VM {
        public:
            VM() { defineNatives(); }
            InterpretResult run();
            InterpretResult interpret(std::string_view source);

        private:
            std::size_t frameCount_{0};
            std::array<CallFrame, FRAME_MAX> frames_{};
            std::array<Value, STACK_SIZE> stack_{};
            Value* stackTop_ {stack_.data()};
            StringPool strPool_{};
            FunctionPool funcPool_{};
            ClosurePool closurePool_{};
            std::vector<ObjUpValue*> openUpvalues_;
            UpvaluePool upvaluePool_{};
            UpvalueIndex openUpvalueHead_{SIZE_MAX};
            ClassPool classPool_{};
            InstancePool instancePool_{};
            BoundMethodPool boundMethodPool_{};
            std::unordered_map<std::string_view, Value> globalVariables_{};
            std::unordered_set<std::string> immutableGlobals_{};

            void push(Value value);
            Value pop();
            Value peek(int distance);
            void binaryOp(BinaryOp op);
            std::string_view extractVariableName(const Value& idenfitier);
            std::size_t readConstantIndexLong(const std::uint8_t* frameIp);
            bool callValue(const Value& callee, std::uint8_t argCount);
            const ObjFunction& currentFunction(const CallFrame& frame);
            void runtimeError(std::string_view errorMessage);
            void defineNatives();
            UpvalueIndex captureUpvalue(Value* local);
            void closeUpvalues(Value* last);
    };
}
