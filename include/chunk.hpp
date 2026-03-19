#pragma once
#include <vector>
#include <cstdint>
#include <cstddef>
#include <variant>

using Value = std::variant<double>;

enum class OpCode : std::uint8_t {
    OP_CONSTANT,
    OP_RETURN
};

class Chunk {
    public:
        Chunk() {
            code.reserve(64);
            line.reserve(64);

        }

        void write(OpCode op, int lineNum) {
            code.push_back(static_cast<std::uint8_t>(op));
            line.push_back(lineNum);
        }

        void write(std::uint8_t byte, int lineNum) {
            code.push_back(byte);
            line.push_back(lineNum);
        }

        std::size_t addConstant(Value value) {
            constants.push_back(value);
            return constants.size() - 1;
        }

        Value getConstant(std::size_t constantIndex) const {
            return constants[constantIndex];
        }

        std::size_t getCodeSize() const {
            return code.size();
        }

        OpCode getInstruction(std::size_t offset) const {
            return static_cast<OpCode>(code[offset]);
        }

        std::uint8_t getRawByte(std::size_t offset) const {
            return code[offset];
        }

        int getLine(std::size_t offset) const {
            return line[offset];
        }

    private:
        std::vector<std::uint8_t> code;
        std::vector<Value> constants;
        std::vector<int> line;
};
