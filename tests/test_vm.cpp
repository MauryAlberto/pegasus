#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "vm.hpp"

using namespace pegasus;

// Helpers
// Interpret a source string and return the result code.
static InterpretResult interpretSource(const std::string& source) {
    Chunk chunk;
    VM vm{chunk};
    return vm.interpret(source);
}

// Build a chunk by hand, run it through the VM, and pop the
// top-of-stack value (returned via OP_RETURN's print, but we
// can also just verify the InterpretResult).
static InterpretResult runChunk(Chunk& chunk) {
    VM vm{chunk};
    return vm.run();
}

// Basic VM execution on hand-built chunks
TEST_CASE("VM executes OP_CONSTANT + OP_RETURN", "[vm]") {
    Chunk chunk;
    chunk.writeConstant(Value{42}, 1);
    chunk.write(OpCode::OP_RETURN, 1);

    REQUIRE(runChunk(chunk) == InterpretResult::OK);
}

TEST_CASE("VM executes OP_NEGATE", "[vm]") {
    Chunk chunk;
    chunk.writeConstant(Value{10}, 1);
    chunk.write(OpCode::OP_NEGATE, 1);
    chunk.write(OpCode::OP_RETURN, 1);

    REQUIRE(runChunk(chunk) == InterpretResult::OK);
}

TEST_CASE("VM executes OP_ADD on two integers", "[vm]") {
    Chunk chunk;
    chunk.writeConstant(Value{3}, 1);
    chunk.writeConstant(Value{4}, 1);
    chunk.write(OpCode::OP_ADD, 1);
    chunk.write(OpCode::OP_RETURN, 1);

    REQUIRE(runChunk(chunk) == InterpretResult::OK);
}

TEST_CASE("VM executes OP_SUBTRACT", "[vm]") {
    Chunk chunk;
    chunk.writeConstant(Value{10}, 1);
    chunk.writeConstant(Value{3}, 1);
    chunk.write(OpCode::OP_SUBTRACT, 1);
    chunk.write(OpCode::OP_RETURN, 1);

    REQUIRE(runChunk(chunk) == InterpretResult::OK);
}

TEST_CASE("VM executes OP_MULTIPLY", "[vm]") {
    Chunk chunk;
    chunk.writeConstant(Value{6}, 1);
    chunk.writeConstant(Value{7}, 1);
    chunk.write(OpCode::OP_MULTIPLY, 1);
    chunk.write(OpCode::OP_RETURN, 1);

    REQUIRE(runChunk(chunk) == InterpretResult::OK);
}

TEST_CASE("VM executes OP_DIVIDE", "[vm]") {
    Chunk chunk;
    chunk.writeConstant(Value{20}, 1);
    chunk.writeConstant(Value{4}, 1);
    chunk.write(OpCode::OP_DIVIDE, 1);
    chunk.write(OpCode::OP_RETURN, 1);

    REQUIRE(runChunk(chunk) == InterpretResult::OK);
}

TEST_CASE("VM handles mixed int/double arithmetic", "[vm]") {
    Chunk chunk;
    chunk.writeConstant(Value{2}, 1);
    chunk.writeConstant(Value{3.5}, 1);
    chunk.write(OpCode::OP_ADD, 1);
    chunk.write(OpCode::OP_RETURN, 1);

    REQUIRE(runChunk(chunk) == InterpretResult::OK);
}

// OP_CONSTANT_LONG
TEST_CASE("VM executes OP_CONSTANT_LONG correctly", "[vm]") {
    Chunk chunk;

    // Fill 256 constant slots
    for (int i = 0; i < 256; ++i)
        chunk.addConstant(Value{i});

    // The 257th constant uses the long encoding
    chunk.writeConstant(Value{777}, 1);
    chunk.write(OpCode::OP_RETURN, 1);

    REQUIRE(runChunk(chunk) == InterpretResult::OK);
}

// Stack overflow / underflow
TEST_CASE("VM detects stack underflow on pop", "[vm]") {
    // A chunk that tries to add without enough operands
    Chunk chunk;
    chunk.writeConstant(Value{1}, 1);
    chunk.write(OpCode::OP_ADD, 1);  // only 1 value on stack, needs 2
    chunk.write(OpCode::OP_RETURN, 1);

    REQUIRE(runChunk(chunk) == InterpretResult::RUNTIME_ERROR);
}

TEST_CASE("VM detects stack overflow", "[vm]") {
    Chunk chunk;
    // Push 1025 values which exceeds STACK_SIZE of 1024 (0 - 1023)
    for (int i = 0; i <= 1024; ++i)
        chunk.writeConstant(Value{i}, 1);
    chunk.write(OpCode::OP_RETURN, 1);

    REQUIRE(runChunk(chunk) == InterpretResult::RUNTIME_ERROR);
}

// Full pipeline: interpret() from source string
TEST_CASE("interpret() returns OK for a valid expression", "[vm]") {
    REQUIRE(interpretSource("1 + 2") == InterpretResult::OK);
}

TEST_CASE("interpret() returns COMPILE_ERROR for bad syntax", "[vm]") {
    REQUIRE(interpretSource("(1 +") == InterpretResult::COMPILE_ERROR);
}

TEST_CASE("interpret() returns OK for complex expression", "[vm]") {
    REQUIRE(interpretSource("(1 + 2) * 3 - 4 / 2") == InterpretResult::OK);
}

TEST_CASE("interpret() handles unary negation", "[vm]") {
    REQUIRE(interpretSource("-42") == InterpretResult::OK);
}

TEST_CASE("interpret() handles nested parens", "[vm]") {
    REQUIRE(interpretSource("((10))") == InterpretResult::OK);
}

TEST_CASE("interpret() handles double negation", "[vm]") {
    REQUIRE(interpretSource("--5") == InterpretResult::OK);
}

TEST_CASE("interpret() returns OK for float arithmetic", "[vm]") {
    REQUIRE(interpretSource("1.5 + 2.5") == InterpretResult::OK);
}

TEST_CASE("interpret() returns OK for mixed int/float expression", "[vm]") {
    REQUIRE(interpretSource("10 / 3.0") == InterpretResult::OK);
}

// Edge: single number
TEST_CASE("interpret() returns OK for a bare number", "[vm]") {
    REQUIRE(interpretSource("99") == InterpretResult::OK);
}
