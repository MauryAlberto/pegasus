#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "compiler.hpp"

using namespace pegasus;

// Helper: compile source and return the populated chunk
// Returns true on success; chunk is populated in-place.
static bool compileSource(std::string_view source, Chunk& chunk) {
    return compile(source, chunk);
}

// Helper: get all opcodes from a chunk
static std::vector<OpCode> opcodes(const Chunk& chunk) {
    std::vector<OpCode> ops;
    std::size_t i = 0;
    while (i < chunk.getCodeSize()) {
        OpCode op = chunk.getInstruction(i);
        ops.push_back(op);
        switch (op) {
            case OpCode::OP_CONSTANT:      i += 2; break;
            case OpCode::OP_CONSTANT_LONG: i += 4; break;
            default:                       i += 1; break;
        }
    }
    return ops;
}

// Number literals
TEST_CASE("Compiler emits constant for integer literal", "[compiler]") {
    Chunk chunk;
    REQUIRE(compileSource("42", chunk));

    auto ops = opcodes(chunk);
    REQUIRE(ops.size() >= 2);
    REQUIRE(ops[0] == OpCode::OP_CONSTANT);
    REQUIRE(ops.back() == OpCode::OP_RETURN);

    std::uint8_t idx = chunk.getRawByte(1);
    REQUIRE(std::get<int>(chunk.getConstant(idx)) == 42);
}

TEST_CASE("Compiler emits constant for floating-point literal", "[compiler]") {
    Chunk chunk;
    REQUIRE(compileSource("3.14", chunk));

    std::uint8_t idx = chunk.getRawByte(1);
    REQUIRE_THAT(std::get<double>(chunk.getConstant(idx)),
                 Catch::Matchers::WithinRel(3.14, 1e-9));
}

// Unary negation
TEST_CASE("Compiler emits OP_NEGATE for unary minus", "[compiler]") {
    Chunk chunk;
    REQUIRE(compileSource("-7", chunk));

    auto ops = opcodes(chunk);
    // OP_CONSTANT, OP_NEGATE, OP_RETURN
    REQUIRE(ops.size() == 3);
    REQUIRE(ops[0] == OpCode::OP_CONSTANT);
    REQUIRE(ops[1] == OpCode::OP_NEGATE);
    REQUIRE(ops[2] == OpCode::OP_RETURN);
}

// Binary arithmetic
TEST_CASE("Compiler emits OP_ADD for addition", "[compiler]") {
    Chunk chunk;
    REQUIRE(compileSource("1 + 2", chunk));

    auto ops = opcodes(chunk);
    // CONST 1, CONST 2, OP_ADD, OP_RETURN
    REQUIRE(ops.size() == 4);
    REQUIRE(ops[0] == OpCode::OP_CONSTANT);
    REQUIRE(ops[1] == OpCode::OP_CONSTANT);
    REQUIRE(ops[2] == OpCode::OP_ADD);
    REQUIRE(ops[3] == OpCode::OP_RETURN);
}

TEST_CASE("Compiler emits OP_SUBTRACT for subtraction", "[compiler]") {
    Chunk chunk;
    REQUIRE(compileSource("5 - 3", chunk));
    auto ops = opcodes(chunk);
    REQUIRE(ops[2] == OpCode::OP_SUBTRACT);
}

TEST_CASE("Compiler emits OP_MULTIPLY for multiplication", "[compiler]") {
    Chunk chunk;
    REQUIRE(compileSource("4 * 6", chunk));
    auto ops = opcodes(chunk);
    REQUIRE(ops[2] == OpCode::OP_MULTIPLY);
}

TEST_CASE("Compiler emits OP_DIVIDE for division", "[compiler]") {
    Chunk chunk;
    REQUIRE(compileSource("8 / 2", chunk));
    auto ops = opcodes(chunk);
    REQUIRE(ops[2] == OpCode::OP_DIVIDE);
}

// logical and comparison
TEST_CASE("Compiler emits OP_NOT for logical !", "[compiler]") {
    Chunk chunk;
    REQUIRE(compileSource("!true", chunk));
    auto ops = opcodes(chunk);
    REQUIRE(ops[1] == OpCode::OP_NOT);
}

TEST_CASE("Compiler emits OP_EQUAL and OP_NOT for comparison !=", "[compiler]") {
    Chunk chunk;
    REQUIRE(compileSource("true != false", chunk));
    auto ops = opcodes(chunk);
    REQUIRE(ops[2] == OpCode::OP_EQUAL);
    REQUIRE(ops[3] == OpCode::OP_NOT);
}

TEST_CASE("Compiler emits OP_EQUAL for comparison ==", "[compiler]") {
    Chunk chunk;
    REQUIRE(compileSource("10 == 10", chunk));
    auto ops = opcodes(chunk);
    REQUIRE(ops[2] == OpCode::OP_EQUAL);
}

TEST_CASE("Compiler emits OP_LESS and OP_NOT for comparison >=", "[compiler]") {
    Chunk chunk;
    REQUIRE(compileSource("10 >= 5", chunk));
    auto ops = opcodes(chunk);
    REQUIRE(ops[2] == OpCode::OP_LESS);
    REQUIRE(ops[3] == OpCode::OP_NOT);
}

TEST_CASE("Compiler emits OP_LESS for  comparison <", "[compiler]") {
    Chunk chunk;
    REQUIRE(compileSource("5 < 10", chunk));
    auto ops = opcodes(chunk);
    REQUIRE(ops[2] == OpCode::OP_LESS);
}

TEST_CASE("Compiler emits OP_GREATER and OP_NOT for comparison <=", "[compiler]") {
    Chunk chunk;
    REQUIRE(compileSource("5 <= 10", chunk));
    auto ops = opcodes(chunk);
    REQUIRE(ops[2] == OpCode::OP_GREATER );
    REQUIRE(ops[3] == OpCode::OP_NOT);
}

// Operator precedence
TEST_CASE("Multiplication binds tighter than addition", "[compiler]") {
    Chunk chunk;
    REQUIRE(compileSource("1 + 2 * 3", chunk));

    auto ops = opcodes(chunk);
    // 1, 2, 3, OP_MULTIPLY, OP_ADD, OP_RETURN
    REQUIRE(ops.size() == 6);
    REQUIRE(ops[0] == OpCode::OP_CONSTANT); // 1
    REQUIRE(ops[1] == OpCode::OP_CONSTANT); // 2
    REQUIRE(ops[2] == OpCode::OP_CONSTANT); // 3
    REQUIRE(ops[3] == OpCode::OP_MULTIPLY);
    REQUIRE(ops[4] == OpCode::OP_ADD);
    REQUIRE(ops[5] == OpCode::OP_RETURN);
}

TEST_CASE("Division binds tighter than subtraction", "[compiler]") {
    Chunk chunk;
    REQUIRE(compileSource("10 - 6 / 3", chunk));
    auto ops = opcodes(chunk);
    REQUIRE(ops[3] == OpCode::OP_DIVIDE);
    REQUIRE(ops[4] == OpCode::OP_SUBTRACT);
}

// Grouping (parentheses)
TEST_CASE("Parentheses override default precedence", "[compiler]") {
    Chunk chunk;
    REQUIRE(compileSource("(1 + 2) * 3", chunk));

    auto ops = opcodes(chunk);
    // 1, 2, OP_ADD, 3, OP_MULTIPLY, OP_RETURN
    REQUIRE(ops.size() == 6);
    REQUIRE(ops[0] == OpCode::OP_CONSTANT); // 1
    REQUIRE(ops[1] == OpCode::OP_CONSTANT); // 2
    REQUIRE(ops[2] == OpCode::OP_ADD);
    REQUIRE(ops[3] == OpCode::OP_CONSTANT); // 3
    REQUIRE(ops[4] == OpCode::OP_MULTIPLY);
    REQUIRE(ops[5] == OpCode::OP_RETURN);
}

TEST_CASE("Nested parentheses compile correctly", "[compiler]") {
    Chunk chunk;
    REQUIRE(compileSource("((1 + 2))", chunk));
    auto ops = opcodes(chunk);
    REQUIRE(ops[0] == OpCode::OP_CONSTANT);
    REQUIRE(ops[1] == OpCode::OP_CONSTANT);
    REQUIRE(ops[2] == OpCode::OP_ADD);
    REQUIRE(ops[3] == OpCode::OP_RETURN);
}

// Complex expressions
TEST_CASE("Compiler handles chained additions", "[compiler]") {
    Chunk chunk;
    REQUIRE(compileSource("1 + 2 + 3", chunk));
    auto ops = opcodes(chunk);
    // Left-associative: (1+2)+3
    // 1, 2, ADD, 3, ADD, RETURN
    REQUIRE(ops.size() == 6);
    REQUIRE(ops[2] == OpCode::OP_ADD);
    REQUIRE(ops[4] == OpCode::OP_ADD);
}

TEST_CASE("Unary minus in expression: -1 + 2", "[compiler]") {
    Chunk chunk;
    REQUIRE(compileSource("-1 + 2", chunk));
    auto ops = opcodes(chunk);
    // CONST(1), NEGATE, CONST(2), ADD, RETURN
    REQUIRE(ops[0] == OpCode::OP_CONSTANT);
    REQUIRE(ops[1] == OpCode::OP_NEGATE);
    REQUIRE(ops[2] == OpCode::OP_CONSTANT);
    REQUIRE(ops[3] == OpCode::OP_ADD);
}

// Error handling
TEST_CASE("Compile fails on empty input", "[compiler]") {
    Chunk chunk;
    // Empty string → advance gets EOF, parsePrecedence fails
    REQUIRE_FALSE(compileSource("", chunk));
}

TEST_CASE("Compile fails on missing closing paren", "[compiler]") {
    Chunk chunk;
    REQUIRE_FALSE(compileSource("(1 + 2", chunk));
}

TEST_CASE("Compile fails on unexpected token", "[compiler]") {
    Chunk chunk;
    REQUIRE_FALSE(compileSource("+ 1", chunk));
}

TEST_CASE("Compile fails on trailing garbage", "[compiler]") {
    Chunk chunk;
    REQUIRE_FALSE(compileSource("1 2", chunk));
}
