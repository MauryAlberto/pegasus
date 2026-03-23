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
    OP_NEGATE,
    OP_RETURN
    };

    struct LineInfo {
        int line;
        std::size_t count;
    };

    class Chunk {
        public:
            Chunk() {
                code_.reserve(64);
                line_.reserve(64);
            }

            void write(OpCode op, int lineNum) {
                code_.push_back(static_cast<std::uint8_t>(op));
                trackLine(lineNum);
            }

            void write(std::uint8_t byte, int lineNum) {
                code_.push_back(byte);
                trackLine(lineNum);
            }

            std::size_t addConstant(Value value) {
                constants_.push_back(value);
                return constants_.size() - 1;
            }

            void writeConstant(Value value, int lineNum) {
                std::size_t constantIndex = addConstant(value);

                if(constantIndex <= 255) {
                    write(OpCode::OP_CONSTANT, lineNum);
                    write(static_cast<std::uint8_t>(constantIndex), lineNum);
                } else {
                    write(OpCode::OP_CONSTANT_LONG, lineNum);
                    write(static_cast<std::uint8_t>((constantIndex & 0xFF)), lineNum);
                    write(static_cast<std::uint8_t>((constantIndex >> 8) & 0xFF), lineNum);
                    write(static_cast<std::uint8_t>((constantIndex >> 16) & 0xFF), lineNum);
                }
            }

            void free() {
                code_.clear();
                constants_.clear();
                line_.clear();
                code_.shrink_to_fit();
                constants_.shrink_to_fit();
                line_.shrink_to_fit();
            }

            Value getConstant(std::size_t constantIndex) const {
                return constants_[constantIndex];
            }

            std::size_t getCodeSize() const {
                return code_.size();
            }

            OpCode getInstruction(std::size_t offset) const {
                return static_cast<OpCode>(code_[offset]);
            }

            std::uint8_t getRawByte(std::size_t offset) const {
                return code_[offset];
            }

            int getLine(std::size_t offset) const {
                std::size_t accumulated{0};
                for(const auto& [lineNum, count] : line_) {
                    accumulated += count;
                    if(offset < accumulated) {
                        return lineNum;
                    }
                }

                return 0;
            }

            const std::uint8_t* getCode() const {
                return code_.data();
            }

        private:
            std::vector<std::uint8_t> code_;
            std::vector<Value> constants_;
            std::vector<LineInfo> line_;

            void trackLine(int lineNum) {
                if(!line_.empty() && line_.back().line == lineNum) {
                    line_.back().count++;
                } else {
                    line_.push_back({lineNum, 1});
                }
            }
    };
}