#include "debug.hpp"

namespace pegasus {
    static std::size_t invokeInstruction(std::string_view name, const Chunk* chunk, std::size_t offset) {
        offset++;
        std::uint8_t nameIndex{chunk->getRawByte(offset++)};
        std::uint8_t argCount{chunk->getRawByte(offset++)};
        Value constantName{chunk->getConstant(nameIndex)};
        printf("%-19s ", name.data());
        printValue(constantName);
        printf(" (%d args)\n", argCount);
        return offset;
    }

    static std::size_t jumpInstruction(std::string_view name, int sign, const Chunk* chunk, std::size_t offset) {
        std::uint8_t lsb{chunk->getRawByte(offset + 1)};
        std::uint8_t msb{chunk->getRawByte(offset + 2)};
        std::uint16_t jump{static_cast<std::uint16_t>((msb << 8) | lsb)};

        std::size_t target{(sign == 1) ? offset + 3 + jump : offset + 3 - jump};
        printf("%-16s %4zu -> %zu\n", name.data(), offset, target);
        return offset + 3;
    }

    static std::size_t byteInstruction(std::string_view name, const Chunk* chunk, std::size_t offset) {
        std::uint8_t slot{chunk->getRawByte(offset + 1)};
        printf("%-16s %4d\n", name.data(), slot);
        return offset + 2;
    }

    static std::size_t byteLongInstruction(std::string_view name, const Chunk* chunk, std::size_t offset) {
        std::size_t slot{
            static_cast<std::size_t>(chunk->getRawByte(offset + 1)) |
            (static_cast<std::size_t>(chunk->getRawByte(offset + 2)) << 8)
        };
        printf("%-16s %4zu\n", name.data(), slot);
        return offset + 3;
    }


    static std::size_t constantInstruction(std::string_view name, const Chunk* chunk, std::size_t offset) {
    std::uint8_t constantIndex{chunk->getRawByte(offset + 1)};
    Value constant{chunk->getConstant(constantIndex)};

    printf("%-19s ", name.data());
    printValue(constant);
    printf("\n");
    return offset + 2;
    }

    static std::size_t constantLongInstruction(std::string_view name, const Chunk* chunk, std::size_t offset) {
        std::size_t constantIndex{
            static_cast<std::size_t>(chunk->getRawByte(offset + 1)) |
            (static_cast<std::size_t>(chunk->getRawByte(offset + 2)) << 8)};
        Value constant{chunk->getConstant(constantIndex)};

        printf("%-16s ", name.data());
        printValue(constant);
        printf("\n");

        return offset + 3;
    }

    static std::size_t simpleInstruction(std::string_view name, std::size_t offset) {
        printf("%s\n", name.data());
        return offset + 1;
    }

    void disassembleChunk(const Chunk* chunk, std::string_view name, const FunctionPool* pool) {
        printf("== %s ==\n", name.data());

        for(std::size_t offset = 0; offset < chunk->getCodeSize();) {
            offset = disassembleInstruction(chunk, offset, pool);
        }
    }

    std::size_t disassembleInstruction(const Chunk* chunk, std::size_t offset, const FunctionPool* pool) {
        printf("%04zu ", offset);

        if(offset > 0 && chunk->getLine(offset) == chunk->getLine(offset - 1)) {
            printf("   | ");
        } else {
            printf("%4d ", chunk->getLine(offset));
        }

        OpCode instruction{chunk->getInstruction(offset)};
        switch(instruction) {
            case OpCode::OP_CONSTANT:
                return constantInstruction("OP_CONSTANT", chunk, offset);
            case OpCode::OP_CONSTANT_LONG:
                return constantLongInstruction("OP_CONSTANT_LONG", chunk, offset);
            case OpCode::OP_ADD:
                return simpleInstruction("OP_ADD", offset);
            case OpCode::OP_SUBTRACT:
                return simpleInstruction("OP_SUBTRACT", offset);
            case OpCode::OP_MULTIPLY:
                return simpleInstruction("OP_MULTIPLY", offset);
            case OpCode::OP_DIVIDE:
                return simpleInstruction("OP_DIVIDE", offset);
            case OpCode::OP_NEGATE:
                return simpleInstruction("OP_NEGATE", offset);
            case OpCode::OP_TRUE:
                return simpleInstruction("OP_TRUE", offset);
            case OpCode::OP_FALSE:
                return simpleInstruction("OP_FALSE", offset);
            case OpCode::OP_NIL:
                return simpleInstruction("OP_NIL", offset);
            case OpCode::OP_NOT:
                return simpleInstruction("OP_NOT", offset);
            case OpCode::OP_EQUAL:
                return simpleInstruction("OP_EQUAL", offset);
            case OpCode::OP_GREATER:
                return simpleInstruction("OP_GREATER", offset);
            case OpCode::OP_LESS:
                return simpleInstruction("OP_LESS", offset);
            case OpCode::OP_PRINT:
                return simpleInstruction("OP_PRINT", offset);
            case OpCode::OP_POP:
                return simpleInstruction("OP_POP", offset);
            case OpCode::OP_DEFINE_GLOBAL:
                return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
            case OpCode::OP_DEFINE_GLOBAL_LONG:
                return constantLongInstruction("OP_DEFINE_GLOBAL_LONG", chunk, offset);
            case OpCode::OP_DEFINE_GLOBAL_IMMUT:
                return constantInstruction("OP_DEFINE_GLOBAL_IMMUT", chunk, offset);
            case OpCode::OP_DEFINE_GLOBAL_IMMUT_LONG:
                return constantLongInstruction("OP_DEFINE_GLOBAL_IMMUT_LONG", chunk, offset);
            case OpCode::OP_GET_GLOBAL:
                return constantInstruction("OP_GET_GLOBAL", chunk, offset);
            case OpCode::OP_GET_GLOBAL_LONG:
                return constantLongInstruction("OP_GET_GLOBAL_LONG", chunk, offset);
            case OpCode::OP_SET_GLOBAL:
                return constantInstruction("OP_SET_GLOBAL", chunk, offset);
            case OpCode::OP_SET_GLOBAL_LONG:
                return constantLongInstruction("OP_SET_GLOBAL_LONG", chunk, offset);
            case OpCode::OP_GET_LOCAL:
                return byteInstruction("OP_GET_LOCAL", chunk, offset);
            case OpCode::OP_GET_LOCAL_LONG:
                return byteLongInstruction("OP_GET_LOCAL_LONG", chunk, offset);
            case OpCode::OP_SET_LOCAL:
                return byteInstruction("OP_SET_LOCAL", chunk, offset);
            case OpCode::OP_SET_LOCAL_LONG:
                return byteLongInstruction("OP_SET_LOCAL_LONG", chunk, offset);
            case OpCode::OP_JUMP:
                return jumpInstruction("OP_JUMP", 1, chunk, offset);
            case OpCode::OP_JUMP_IF_FALSE:
                return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
            case OpCode::OP_LOOP:
                return jumpInstruction("OP_LOOP", -1, chunk, offset);
            case OpCode::OP_CALL:
                return byteInstruction("OP_CALL", chunk, offset);
            case OpCode::OP_RETURN:
                return simpleInstruction("OP_RETURN", offset);
            case OpCode::OP_CLOSURE: {
                offset++;
                std::uint8_t constantIndex = chunk->getRawByte(offset++);
                Value constant = chunk->getConstant(constantIndex);
                printf("%-19s ", "OP_CLOSURE");
                printValue(constant);
                printf("\n");

                if(pool && std::holds_alternative<FunctionIndex>(constant)) {
                    FunctionIndex funcIdx = std::get<FunctionIndex>(constant);
                    const ObjFunction& fn = pool->getFunction(funcIdx);
                    for(std::size_t i = 0; i < fn.upvalueCount; i++) {
                        std::uint8_t isLocal = chunk->getRawByte(offset);
                        std::uint8_t index = chunk->getRawByte(offset + 1);
                        printf("%04zu    |                     %s %d\n", 
                            offset, isLocal ? "local" : "upvalue", index);
                        offset += 2;
                    }
                }

                return offset;
            }
            case OpCode::OP_GET_UPVALUE:
                return byteInstruction("OP_GET_UPVALUE", chunk, offset);
            case OpCode::OP_SET_UPVALUE:
                return byteInstruction("OP_SET_UPVALUE", chunk, offset);
            case OpCode::OP_CLOSE_UPVALUE:
                return simpleInstruction("OP_CLOSE_UPVALUE", offset);
            case OpCode::OP_CLASS:
                return constantInstruction("OP_CLASS", chunk, offset);
            case OpCode::OP_GET_PROPERTY:
                return constantInstruction("OP_GET_PROPERTY", chunk, offset);
            case OpCode::OP_SET_PROPERTY:
                return constantInstruction("OP_SET_PROPERTY", chunk, offset);
            case OpCode::OP_METHOD:
                return constantInstruction("OP_METHOD", chunk, offset);
            case OpCode::OP_INVOKE:
                return invokeInstruction("OP_INVOKE", chunk, offset);
            case OpCode::OP_INHERIT:
                return simpleInstruction("OP_INHERIT", offset);
            case OpCode::OP_GET_SUPER:
                return constantInstruction("OP_GET_SUPER", chunk, offset);
            case OpCode::OP_SUPER_INVOKE:
                return invokeInstruction("OP_SUPER_INVOKE", chunk, offset);
            case OpCode::OP_ARRAY:
                return byteInstruction("OP_ARRAY", chunk, offset);
            case OpCode::OP_GET_INDEX:
                return simpleInstruction("OP_GET_INDEX", offset);
            case OpCode::OP_SET_INDEX:
                return simpleInstruction("OP_SET_INDEX", offset);
            case OpCode::OP_ARRAY_LEN:
                return simpleInstruction("OP_ARRAY_LEN", offset);
            case OpCode::OP_ARRAY_PUSH:
                return simpleInstruction("OP_ARRAY_PUSH", offset);
            default:
                printf("unknown opcode %d\n", static_cast<int>(instruction));
                return offset + 1;
        }
    }
}
