#include "chunk.hpp"
#include "debug.hpp"

int main() {
    Chunk chunk;
    chunk.write(OpCode::OP_RETURN);
    disassembleChunk(chunk, "test chunk");
    return 0;
}