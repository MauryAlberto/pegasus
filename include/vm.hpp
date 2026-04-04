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

namespace pegasus {
    inline constexpr int STACK_SIZE = 1024;

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

    class stringPool {
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

    class VM {
        public:
            VM() = delete;
            explicit VM(Chunk& chunk) : chunk_{&chunk}, ip_{chunk_->getCode()} {}
            InterpretResult run();
            InterpretResult interpret(std::string_view source);

        private:
            Chunk* chunk_{nullptr};
            const std::uint8_t* ip_{nullptr};
            std::array<Value, STACK_SIZE> stack_{};
            Value* stackTop_ {stack_.data()};
            stringPool pool_{};
            std::unordered_map<std::string_view, Value> globalVariables_{};

            void push(Value value);
            Value pop();
            Value peek(int distance);
            void binaryOp(BinaryOp op);
    };
}
