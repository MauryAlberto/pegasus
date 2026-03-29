#pragma once
#include <vector>
#include <cstdint>
#include <cstddef>
#include <variant>
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
            void writeConstant(Value value, int lineNum);
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