#pragma once
#include <memory>
#include <array>
#include <type_traits>
#include "chunk.hpp"
#include "debug.hpp"
#include "value.hpp"

namespace pegasus {
    inline constexpr int STACK_SIZE = 256;

    enum class InterpretResult {
        OK,
        COMPILE_ERROR,
        RUNTIME_ERROR
    };

    class VM {
        public:
            VM() = delete;
            explicit VM(const Chunk& chunk) : chunk_{chunk}, ip_{chunk_.getCode()} {}

            InterpretResult run() {
                while(true) {
                    if constexpr(DEBUG_TRACE_EXECUTION) {
                        for(Value* slot {stack_.data()}; slot < stackTop_; slot++) {
                            printf("     ");
                            printf("[ ");
                            printValue(*slot);
                            printf(" ]");
                        }
                        printf("\n");
                        disassembleInstruction(chunk_, static_cast<std::size_t>(ip_ - chunk_.getCode()));
                    }

                    OpCode instruction{static_cast<OpCode>(*ip_++)};
                    switch(instruction) {
                        case OpCode::OP_CONSTANT: {
                            const std::uint8_t constantIndex{*ip_++};
                            Value constant{chunk_.getConstant(constantIndex)};
                            push(constant);
                            break;
                        }
                        case OpCode::OP_CONSTANT_LONG: {
                            const std::size_t lowByte{static_cast<std::size_t>(*ip_++)};
                            const std::size_t midByte{static_cast<std::size_t>(*ip_++)};
                            const std::size_t highByte{static_cast<std::size_t>(*ip_++)};
                            const std::size_t constantIndex{(highByte << 16) | (midByte << 8) | lowByte};
                            Value constant{chunk_.getConstant(constantIndex)};
                            push(constant);
                            break;
                        }
                        case OpCode::OP_NEGATE: {
                            push(negateValue(pop()));
                            break;
                        }
                        case OpCode::OP_RETURN:
                            printValue(pop());
                            printf("\n");
                            return InterpretResult::OK;
                        default:
                            return InterpretResult::RUNTIME_ERROR;
                    }
                }
            }

            void push(Value value) {
                *stackTop_ = value;
                stackTop_++;
            }

            Value pop() {
                stackTop_--;
                return *stackTop_;
            }

        private:
            const Chunk& chunk_;
            const std::uint8_t* ip_;
            std::array<Value, STACK_SIZE> stack_;
            Value* stackTop_ {stack_.data()};
    };
}
