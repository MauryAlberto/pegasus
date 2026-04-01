#pragma once
#include <array>
#include <stdexcept>
#include <type_traits>
#include <string_view>
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

            void push(Value value);
            Value pop();
            void binaryOp(BinaryOp op);
    };
}
