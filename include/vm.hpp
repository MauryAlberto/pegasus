#pragma once
#include <array>
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
        ObjFunction function_;
        const uint8_t* ip_;
        Value* slots_;
    };

    class VM {
        public:
            InterpretResult run();
            InterpretResult interpret(std::string_view source);

        private:
            std::size_t frameCount_{0};
            std::array<CallFrame, FRAME_MAX> frames_{};
            std::array<Value, STACK_SIZE> stack_{};
            Value* stackTop_ {stack_.data()};
            StringPool pool_{};
            std::unordered_map<std::string_view, Value> globalVariables_{};

            void push(Value value);
            Value pop();
            Value peek(int distance);
            void binaryOp(BinaryOp op);
            std::string_view extractVariableName(const Value& idenfitier);
            std::size_t readConstantIndexLong(const std::uint8_t* frameIp);

    };
}
