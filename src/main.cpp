#include "chunk.hpp"
#include "vm.hpp"
#include "debug.hpp"

int main() {
    
    Chunk chunk;
    std::uint8_t constantIndex{static_cast<std::uint8_t>(chunk.addConstant(Value{1.2}))};
    chunk.write(OpCode::OP_CONSTANT, 123);
    chunk.write(constantIndex, 123);
    chunk.write(OpCode::OP_RETURN, 123);
    disassembleChunk(chunk, "test chunk");
    return 0;
}