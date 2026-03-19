#include "debug.hpp"

static std::size_t constantInstruction(const Chunk& chunk, const std::string& name, std::size_t offset) {
    std::uint8_t constantIndex = chunk.getRawByte(offset + 1);
    Value constant = chunk.getConstant(constantIndex);

    printf("%-16s ", name.c_str());

    std::visit([](auto value) {
        using T = decltype(value);
        if constexpr(std::is_same_v<T, double>) {
            printf("%.2f\n", value);
        } else {
            printf("unknown value type\n");
        }
    }, constant);

    return offset + 2;
}

static std::size_t simpleInstruction(const std::string& name, std::size_t offset) {
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

    if(offset > 0 && chunk.getLine(offset) == chunk.getLine(offset - 1)) {
        printf("   | ");
    } else {
        printf("%4d ", chunk.getLine(offset));
    }

    OpCode instruction = chunk.getInstruction(offset);
    switch(instruction) {
        case OpCode::OP_CONSTANT:
            return constantInstruction(chunk, "OP_CONSTANT", offset);
        case OpCode::OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        default:
            printf("unknown opcode %d\n", static_cast<int>(instruction));
            return offset + 1;
    }
}