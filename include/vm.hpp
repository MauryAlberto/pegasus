#pragma once
#include <memory>
#include "chunk.hpp"
#include "debug.hpp"

namespace pegasus {
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
                        disassembleInstruction(chunk_, static_cast<std::size_t>(ip_ - chunk_.getCode()));
                    }

                    OpCode instruction{static_cast<OpCode>(*ip_++)};
                    switch(instruction) {
                        case OpCode::OP_CONSTANT: {
                            const std::uint8_t constantIndex{*ip_++};
                            Value constant{chunk_.getConstant(constantIndex)};
                            printValue(constant);
                            break;
                        }
                        case OpCode::OP_CONSTANT_LONG: {
                            const std::size_t lowByte{static_cast<std::size_t>(*ip_++)};
                            const std::size_t midByte{static_cast<std::size_t>(*ip_++)};
                            const std::size_t highByte{static_cast<std::size_t>(*ip_++)};
                            const std::size_t constantIndex{(highByte << 16) | (midByte << 8) | lowByte};
                            Value constant{chunk_.getConstant(constantIndex)};
                            printValue(constant);
                            break;
                        }
                        case OpCode::OP_RETURN:
                            return InterpretResult::OK;
                        default:
                            return InterpretResult::RUNTIME_ERROR;
                    }
                }
            }

        private:
            const Chunk& chunk_;
            const std::uint8_t* ip_;
    };
}
