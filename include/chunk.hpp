#pragma once
#include <vector>
#include <cstdint>
#include <cstddef>

enum class OpCode : std::uint8_t {
    OP_RETURN
};

class Chunk {
    public:
        Chunk() {
            code.reserve(64);
            line.reserve(64);
        }

        void write(OpCode op) {
            code.push_back(static_cast<std::uint8_t>(op));
        }

        void write(std::uint8_t byte) {
            code.push_back(byte);
        }

        std::size_t getCodeSize() const {
            return code.size();
        }

        OpCode getInstruction(std::size_t offset) const {
            return static_cast<OpCode>(code[offset]);
        }

    private:
        std::vector<std::uint8_t> code;
        std::vector<int> line;
};
