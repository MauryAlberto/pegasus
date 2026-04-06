#pragma once
#include <vector>
#include <cstdint>
#include <cstddef>
#include <variant>
#include <type_traits>
#include "value.hpp"

namespace pegasus {
    enum class OpCode : std::uint8_t {
        OP_CONSTANT,
        OP_CONSTANT_LONG,
        OP_ADD,
        OP_SUBTRACT,
        OP_MULTIPLY,
        OP_DIVIDE,
        OP_NEGATE,
        OP_NIL,
        OP_TRUE,
        OP_FALSE,
        OP_NOT,
        OP_EQUAL,
        OP_GREATER,
        OP_LESS,
        OP_PRINT,
        OP_POP,
        OP_DEFINE_GLOBAL,
        OP_DEFINE_GLOBAL_LONG,
        OP_GET_GLOBAL,
        OP_GET_GLOBAL_LONG,
        OP_SET_GLOBAL,
        OP_SET_GLOBAL_LONG,
        OP_GET_LOCAL,
        OP_GET_LOCAL_LONG,
        OP_SET_LOCAL,
        OP_SET_LOCAL_LONG,
        OP_RETURN
    };

    struct LineInfo {
        int line;
        std::size_t count;
    };

    class Chunk {
        public:
            Chunk();
            void write(OpCode op, int lineNum);
            void write(std::uint8_t byte, int lineNum);
            std::size_t writeConstant(Value value, int lineNum);
            std::size_t addConstant(Value value);
            Value getConstant(std::size_t constantIndex) const;
            const std::uint8_t* getCode() const;
            std::size_t getCodeSize() const;
            OpCode getInstruction(std::size_t offset) const;
            std::uint8_t getRawByte(std::size_t offset) const;
            int getLine(std::size_t offset) const;
            void free();

        private:
            std::vector<std::uint8_t> code_;
            std::vector<Value> constants_;
            std::vector<LineInfo> line_;

            void trackLine(int lineNum);
    };
}