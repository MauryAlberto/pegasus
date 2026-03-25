#include "chunk.hpp"
#include "vm.hpp"
#include "debug.hpp"

int main() {
    pegasus::Chunk chunk;
    chunk.writeConstant(pegasus::Value{1.2}, 123);
    chunk.writeConstant(pegasus::Value{3.4}, 123);
    chunk.write(pegasus::OpCode::OP_ADD, 123);
    
    chunk.writeConstant(pegasus::Value{5.6}, 123);
    chunk.write(pegasus::OpCode::OP_DIVIDE, 123);
    chunk.write(pegasus::OpCode::OP_NEGATE, 123);
    chunk.write(pegasus::OpCode::OP_RETURN, 123);
    
    pegasus::VM vm{chunk};
    vm.run();
    return 0;
}