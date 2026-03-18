#include "debug.hpp"

std::size_t simpleInstruction(const std::string& name, std::size_t offset) {
    printf("%s\n", name.c_str());
    return offset + 1;
}

void disassembleChunk(const Chunk& chunk, const std::string& name) {
    printf("== %s ==\n", name.c_str());

    for(std::size_t offset = 0; offset < chunk.getCodeSize();) {
        offset = disassembleInstruction(chunk, offset);
    }
}

std::size_t disassembleInstruction(const Chunk& chunk, std::size_t offset) {
    printf("%04zu ", offset);

    OpCode instruction = chunk.getInstruction(offset);
    switch(instruction) {
        case OpCode::OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        default:
            printf("Unkown opcode %d\n", static_cast<int>(instruction));
            return offset + 1;
    }
}