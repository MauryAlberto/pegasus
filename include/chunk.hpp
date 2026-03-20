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

struct LineInfo {
    int line;
    std::size_t count;
};

class Chunk {
    public:
        Chunk() {
            code.reserve(64);
            line.reserve(64);
        }

        void write(OpCode op, int lineNum) {
            code.push_back(static_cast<std::uint8_t>(op));
            trackLine(lineNum);
        }

        void write(std::uint8_t byte, int lineNum) {
            code.push_back(byte);
            trackLine(lineNum);
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
            std::size_t accumulated{0};
            for(const auto& [lineNum, count] : line) {
                accumulated += count;
                if(offset < accumulated) {
                    return lineNum;
                }
            }

            return 0;
        }

    private:
        std::vector<std::uint8_t> code;
        std::vector<Value> constants;
        std::vector<LineInfo> line;

        void trackLine(int lineNum) {
            if(!line.empty() && line.back().line == lineNum) {
                line.back().count++;
            } else {
                line.push_back({lineNum, 1});
            }
        }
};
