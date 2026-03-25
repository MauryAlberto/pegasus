#include "chunk.hpp"
#include "vm.hpp"
#include "debug.hpp"

int main() {
    
    pegasus::Chunk chunk;
    std::uint8_t constantIndex{static_cast<std::uint8_t>(chunk.addConstant(pegasus::Value{1.2}))};
    chunk.write(pegasus::OpCode::OP_CONSTANT, 123);
    chunk.write(constantIndex, 123);
    chunk.write(pegasus::OpCode::OP_NEGATE, 123);
    chunk.write(pegasus::OpCode::OP_RETURN, 123);
    pegasus::VM vm{chunk};
    vm.run();
    // pegasus::disassembleChunk(chunk, "test chunk");
    return 0;
}