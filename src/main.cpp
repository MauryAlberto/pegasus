#include "chunk.hpp"
#include "debug.hpp"

int main() {
    Chunk chunk;
    std::uint8_t constantIndex = static_cast<std::uint8_t>(chunk.addConstant(Value{1.2}));
    chunk.write(OpCode::OP_CONSTANT);
    chunk.write(constantIndex);
    return 0;
}