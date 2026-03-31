#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "chunk.hpp"

using namespace pegasus;

// Construction & Initial State
TEST_CASE("Chunk default-constructs empty", "[chunk]") {
    Chunk chunk;
    REQUIRE(chunk.getCodeSize() == 0);
}

// write() – OpCode and raw byte overloads
TEST_CASE("write(OpCode) appends an instruction", "[chunk]") {
    Chunk chunk;
    chunk.write(OpCode::OP_RETURN, 1);
    REQUIRE(chunk.getCodeSize() == 1);
    REQUIRE(chunk.getInstruction(0) == OpCode::OP_RETURN);
}

TEST_CASE("write(uint8_t) appends a raw byte", "[chunk]") {
    Chunk chunk;
    chunk.write(static_cast<std::uint8_t>(0xAB), 1);
    REQUIRE(chunk.getCodeSize() == 1);
    REQUIRE(chunk.getRawByte(0) == 0xAB);
}

TEST_CASE("Multiple writes grow the bytecode stream", "[chunk]") {
    Chunk chunk;
    chunk.write(OpCode::OP_CONSTANT, 1);
    chunk.write(static_cast<std::uint8_t>(0), 1);
    chunk.write(OpCode::OP_RETURN, 1);
    REQUIRE(chunk.getCodeSize() == 3);
    REQUIRE(chunk.getInstruction(0) == OpCode::OP_CONSTANT);
    REQUIRE(chunk.getRawByte(1) == 0);
    REQUIRE(chunk.getInstruction(2) == OpCode::OP_RETURN);
}

// addConstant() & getConstant()
TEST_CASE("addConstant returns ascending indices", "[chunk]") {
    Chunk chunk;
    REQUIRE(chunk.addConstant(Value{42}) == 0);
    REQUIRE(chunk.addConstant(Value{3.14}) == 1);
    REQUIRE(chunk.addConstant(Value{99}) == 2);
}

TEST_CASE("getConstant retrieves the correct value", "[chunk]") {
    Chunk chunk;
    chunk.addConstant(Value{7});
    chunk.addConstant(Value{2.718});

    REQUIRE(std::get<int>(chunk.getConstant(0)) == 7);
    REQUIRE_THAT(std::get<double>(chunk.getConstant(1)),
                 Catch::Matchers::WithinRel(2.718, 1e-9));
}

// writeConstant() – short path (index <= 255)
TEST_CASE("writeConstant emits OP_CONSTANT for small index", "[chunk]") {
    Chunk chunk;
    chunk.writeConstant(Value{100}, 1);

    REQUIRE(chunk.getInstruction(0) == OpCode::OP_CONSTANT);
    std::uint8_t idx = chunk.getRawByte(1);
    REQUIRE(std::get<int>(chunk.getConstant(idx)) == 100);
}

// ============================================================
// writeConstant() – long path (index > 255)
// ============================================================

TEST_CASE("writeConstant emits OP_CONSTANT_LONG for large index", "[chunk]") {
    Chunk chunk;

    // Fill the first 256 constant slots
    for (int i = 0; i < 256; ++i) {
        chunk.addConstant(Value{i});
    }

    // The 257th constant (index 256) must use the long encoding
    chunk.writeConstant(Value{9999}, 5);

    REQUIRE(chunk.getInstruction(0) == OpCode::OP_CONSTANT_LONG);

    // Reconstruct the 24-bit index from the three following bytes
    std::size_t lo  = chunk.getRawByte(1);
    std::size_t mid = chunk.getRawByte(2);
    std::size_t hi  = chunk.getRawByte(3);
    std::size_t reconstructed = lo | (mid << 8) | (hi << 16);
    REQUIRE(reconstructed == 256);
    REQUIRE(std::get<int>(chunk.getConstant(reconstructed)) == 9999);
}

// Line-number tracking (RLE)
TEST_CASE("getLine returns correct line for sequential writes", "[chunk]") {
    Chunk chunk;
    chunk.write(OpCode::OP_RETURN, 1);
    chunk.write(OpCode::OP_RETURN, 1);
    chunk.write(OpCode::OP_RETURN, 2);
    chunk.write(OpCode::OP_RETURN, 3);

    REQUIRE(chunk.getLine(0) == 1);
    REQUIRE(chunk.getLine(1) == 1);
    REQUIRE(chunk.getLine(2) == 2);
    REQUIRE(chunk.getLine(3) == 3);
}

TEST_CASE("getLine handles RLE compression correctly", "[chunk]") {
    Chunk chunk;
    // 5 bytes all on line 10
    for (int i = 0; i < 5; ++i)
        chunk.write(OpCode::OP_RETURN, 10);
    // 3 bytes on line 20
    for (int i = 0; i < 3; ++i)
        chunk.write(OpCode::OP_RETURN, 20);

    for (int i = 0; i < 5; ++i) REQUIRE(chunk.getLine(i) == 10);
    for (int i = 5; i < 8; ++i) REQUIRE(chunk.getLine(i) == 20);
}

// free() resets everything
TEST_CASE("free() clears code, constants, and lines", "[chunk]") {
    Chunk chunk;
    chunk.writeConstant(Value{42}, 1);
    chunk.write(OpCode::OP_RETURN, 1);
    REQUIRE(chunk.getCodeSize() > 0);

    chunk.free();
    REQUIRE(chunk.getCodeSize() == 0);
}

// getCode() pointer validity
TEST_CASE("getCode returns pointer to first byte", "[chunk]") {
    Chunk chunk;
    chunk.write(OpCode::OP_NEGATE, 1);
    const std::uint8_t* code = chunk.getCode();
    REQUIRE(code != nullptr);
    REQUIRE(*code == static_cast<std::uint8_t>(OpCode::OP_NEGATE));
}
