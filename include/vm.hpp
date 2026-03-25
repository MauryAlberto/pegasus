#pragma once
#include <array>
#include <stdexcept>
#include <type_traits>
#include <string_view>
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

    enum class BinaryOp {
        ADD,
        SUBTRACT,
        MULTIPLY,
        DIVIDE
    };

    class VM {
        public:
            VM() = delete;
            explicit VM(const Chunk& chunk) : chunk_{chunk}, ip_{chunk_.getCode()} {}

            InterpretResult run() {
                try {
                    while(true) {
                        if constexpr(DEBUG_TRACE_EXECUTION) {
                            printf("%-10s", "");
                            for(Value* slot {stack_.data()}; slot < stackTop_; slot++) {
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
                            case OpCode::OP_ADD:        {binaryOp(BinaryOp::ADD); break;}
                            case OpCode::OP_SUBTRACT:   {binaryOp(BinaryOp::SUBTRACT); break;}
                            case OpCode::OP_MULTIPLY:   {binaryOp(BinaryOp::MULTIPLY); break;}
                            case OpCode::OP_DIVIDE:     {binaryOp(BinaryOp::DIVIDE); break;}
                            case OpCode::OP_NEGATE:     {push(negateValue(pop()));break;}
                            case OpCode::OP_RETURN:
                                printValue(pop());
                                printf("\n");
                                return InterpretResult::OK;
                            default:
                                return InterpretResult::RUNTIME_ERROR;
                        }
                    }
                } catch (const std::runtime_error& e) {
                    printf("runtime error: %s\n", e.what());
                    return InterpretResult::RUNTIME_ERROR;
                }
            }
            
            static InterpretResult interpret(std::string_view source) {
                // todo: compile source to bytecode and then run
                static_cast<void>(source);
                return InterpretResult::OK;
            }

        private:
            const Chunk& chunk_;
            const std::uint8_t* ip_;
            std::array<Value, STACK_SIZE> stack_;
            Value* stackTop_ {stack_.data()};

            void push(Value value) {
                if(stackTop_ >= stack_.data() + STACK_SIZE) {
                    throw std::runtime_error("stack overflow");
                }
                *stackTop_ = value;
                stackTop_++;
            }

            Value pop() {
                if(stackTop_ == stack_.data()) {
                    throw std::runtime_error("stack underflow");
                }
                stackTop_--;
                return *stackTop_;
            } 

            void binaryOp(BinaryOp op) {
                Value b{pop()};
                Value a{pop()};
                
                std::visit([&op, this](auto aVal, auto bVal) {
                    using A = decltype(aVal);
                    using B = decltype(bVal);

                    if constexpr(std::is_arithmetic_v<A> && std::is_arithmetic_v<B>) {
                        switch(op) {
                            case BinaryOp::ADD:         {push(Value{aVal + bVal});break;}
                            case BinaryOp::SUBTRACT:    {push(Value{aVal - bVal}); break;}
                            case BinaryOp::MULTIPLY:    {push(Value{aVal * bVal}); break;}
                            case BinaryOp::DIVIDE:      {push(Value{aVal / bVal}); break;}
                            default:
                                throw std::runtime_error("unknown binary operator");
                        }
                    } else {
                        throw std::runtime_error("operands must be numbers");
                    }

                }, a, b);
            }
    };
}
