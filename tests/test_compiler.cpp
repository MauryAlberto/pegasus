#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "compiler.hpp"

using namespace pegasus;

// Helper: compile source and return the ObjFunction
static std::optional<ObjFunction> compileSource(std::string_view source) {
    FunctionPool funcPool;
    return compile(source, funcPool);
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
    auto fn = compileSource("42;");
    REQUIRE(fn.has_value());

    auto ops = opcodes(fn->chunk);
    REQUIRE(ops.size() >= 2);
    REQUIRE(ops[0] == OpCode::OP_CONSTANT);
    REQUIRE(ops.back() == OpCode::OP_RETURN);

    std::uint8_t idx = fn->chunk.getRawByte(1);
    REQUIRE(std::get<int>(fn->chunk.getConstant(idx)) == 42);
}

TEST_CASE("Compiler emits constant for floating-point literal", "[compiler]") {
    auto fn = compileSource("3.14;");
    REQUIRE(fn.has_value());

    std::uint8_t idx = fn->chunk.getRawByte(1);
    REQUIRE_THAT(std::get<double>(fn->chunk.getConstant(idx)),
                 Catch::Matchers::WithinRel(3.14, 1e-9));
}

// Unary negation
TEST_CASE("Compiler emits OP_NEGATE for unary minus", "[compiler]") {
    auto fn = compileSource("-7;");
    REQUIRE(fn.has_value());

    auto ops = opcodes(fn->chunk);
    REQUIRE(ops.size() == 4);
    REQUIRE(ops[0] == OpCode::OP_CONSTANT);
    REQUIRE(ops[1] == OpCode::OP_NEGATE);
    REQUIRE(ops[2] == OpCode::OP_POP);
    REQUIRE(ops[3] == OpCode::OP_RETURN);
}

// Binary arithmetic
TEST_CASE("Compiler emits OP_ADD for addition", "[compiler]") {
    auto fn = compileSource("1 + 2;");
    REQUIRE(fn.has_value());

    auto ops = opcodes(fn->chunk);
    REQUIRE(ops.size() == 5);
    REQUIRE(ops[0] == OpCode::OP_CONSTANT);
    REQUIRE(ops[1] == OpCode::OP_CONSTANT);
    REQUIRE(ops[2] == OpCode::OP_ADD);
    REQUIRE(ops[3] == OpCode::OP_POP);
    REQUIRE(ops[4] == OpCode::OP_RETURN);
}

TEST_CASE("Compiler emits OP_SUBTRACT for subtraction", "[compiler]") {
    auto fn = compileSource("5 - 3;");
    REQUIRE(fn.has_value());
    auto ops = opcodes(fn->chunk);
    REQUIRE(ops[2] == OpCode::OP_SUBTRACT);
}

TEST_CASE("Compiler emits OP_MULTIPLY for multiplication", "[compiler]") {
    auto fn = compileSource("4 * 6;");
    REQUIRE(fn.has_value());
    auto ops = opcodes(fn->chunk);
    REQUIRE(ops[2] == OpCode::OP_MULTIPLY);
}

TEST_CASE("Compiler emits OP_DIVIDE for division", "[compiler]") {
    auto fn = compileSource("8 / 2;");
    REQUIRE(fn.has_value());
    auto ops = opcodes(fn->chunk);
    REQUIRE(ops[2] == OpCode::OP_DIVIDE);
}

// Logical and comparison
TEST_CASE("Compiler emits OP_NOT for logical !", "[compiler]") {
    auto fn = compileSource("!true;");
    REQUIRE(fn.has_value());
    auto ops = opcodes(fn->chunk);
    REQUIRE(ops[1] == OpCode::OP_NOT);
}

TEST_CASE("Compiler emits OP_EQUAL and OP_NOT for comparison !=", "[compiler]") {
    auto fn = compileSource("true != false;");
    REQUIRE(fn.has_value());
    auto ops = opcodes(fn->chunk);
    REQUIRE(ops[2] == OpCode::OP_EQUAL);
    REQUIRE(ops[3] == OpCode::OP_NOT);
}

TEST_CASE("Compiler emits OP_EQUAL for comparison ==", "[compiler]") {
    auto fn = compileSource("10 == 10;");
    REQUIRE(fn.has_value());
    auto ops = opcodes(fn->chunk);
    REQUIRE(ops[2] == OpCode::OP_EQUAL);
}

TEST_CASE("Compiler emits OP_LESS and OP_NOT for comparison >=", "[compiler]") {
    auto fn = compileSource("10 >= 5;");
    REQUIRE(fn.has_value());
    auto ops = opcodes(fn->chunk);
    REQUIRE(ops[2] == OpCode::OP_LESS);
    REQUIRE(ops[3] == OpCode::OP_NOT);
}

TEST_CASE("Compiler emits OP_LESS for  comparison <", "[compiler]") {
    auto fn = compileSource("5 < 10;");
    REQUIRE(fn.has_value());
    auto ops = opcodes(fn->chunk);
    REQUIRE(ops[2] == OpCode::OP_LESS);
}

TEST_CASE("Compiler emits OP_GREATER and OP_NOT for comparison <=", "[compiler]") {
    auto fn = compileSource("5 <= 10;");
    REQUIRE(fn.has_value());
    auto ops = opcodes(fn->chunk);
    REQUIRE(ops[2] == OpCode::OP_GREATER);
    REQUIRE(ops[3] == OpCode::OP_NOT);
}

// Operator precedence
TEST_CASE("Multiplication binds tighter than addition", "[compiler]") {
    auto fn = compileSource("1 + 2 * 3;");
    REQUIRE(fn.has_value());

    auto ops = opcodes(fn->chunk);
    REQUIRE(ops.size() == 7);
    REQUIRE(ops[0] == OpCode::OP_CONSTANT); // 1
    REQUIRE(ops[1] == OpCode::OP_CONSTANT); // 2
    REQUIRE(ops[2] == OpCode::OP_CONSTANT); // 3
    REQUIRE(ops[3] == OpCode::OP_MULTIPLY);
    REQUIRE(ops[4] == OpCode::OP_ADD);
    REQUIRE(ops[5] == OpCode::OP_POP);
    REQUIRE(ops[6] == OpCode::OP_RETURN);
}

TEST_CASE("Division binds tighter than subtraction", "[compiler]") {
    auto fn = compileSource("10 - 6 / 3;");
    REQUIRE(fn.has_value());
    auto ops = opcodes(fn->chunk);
    REQUIRE(ops[3] == OpCode::OP_DIVIDE);
    REQUIRE(ops[4] == OpCode::OP_SUBTRACT);
}

// Grouping (parentheses)
TEST_CASE("Parentheses override default precedence", "[compiler]") {
    auto fn = compileSource("(1 + 2) * 3;");
    REQUIRE(fn.has_value());

    auto ops = opcodes(fn->chunk);
    REQUIRE(ops.size() == 7);
    REQUIRE(ops[0] == OpCode::OP_CONSTANT); // 1
    REQUIRE(ops[1] == OpCode::OP_CONSTANT); // 2
    REQUIRE(ops[2] == OpCode::OP_ADD);
    REQUIRE(ops[3] == OpCode::OP_CONSTANT); // 3
    REQUIRE(ops[4] == OpCode::OP_MULTIPLY);
    REQUIRE(ops[5] == OpCode::OP_POP);
    REQUIRE(ops[6] == OpCode::OP_RETURN);
}

TEST_CASE("Nested parentheses compile correctly", "[compiler]") {
    auto fn = compileSource("((1 + 2));");
    REQUIRE(fn.has_value());
    auto ops = opcodes(fn->chunk);
    REQUIRE(ops[0] == OpCode::OP_CONSTANT);
    REQUIRE(ops[1] == OpCode::OP_CONSTANT);
    REQUIRE(ops[2] == OpCode::OP_ADD);
    REQUIRE(ops[3] == OpCode::OP_POP);
    REQUIRE(ops[4] == OpCode::OP_RETURN);
}

// Complex expressions
TEST_CASE("Compiler handles chained additions", "[compiler]") {
    auto fn = compileSource("1 + 2 + 3;");
    REQUIRE(fn.has_value());
    auto ops = opcodes(fn->chunk);
    REQUIRE(ops.size() == 7);
    REQUIRE(ops[2] == OpCode::OP_ADD);
    REQUIRE(ops[4] == OpCode::OP_ADD);
}

TEST_CASE("Unary minus in expression: -1 + 2", "[compiler]") {
    auto fn = compileSource("-1 + 2;");
    REQUIRE(fn.has_value());
    auto ops = opcodes(fn->chunk);
    REQUIRE(ops[0] == OpCode::OP_CONSTANT);
    REQUIRE(ops[1] == OpCode::OP_NEGATE);
    REQUIRE(ops[2] == OpCode::OP_CONSTANT);
    REQUIRE(ops[3] == OpCode::OP_ADD);
}

// Error handling
TEST_CASE("Compile fails on empty input", "[compiler]") {
    REQUIRE_FALSE(compileSource(";").has_value());
}

TEST_CASE("Compile fails on missing closing paren", "[compiler]") {
    REQUIRE_FALSE(compileSource("(1 + 2;").has_value());
}

TEST_CASE("Compile fails on unexpected token", "[compiler]") {
    REQUIRE_FALSE(compileSource("+ 1;").has_value());
}

TEST_CASE("Compile fails on trailing garbage", "[compiler]") {
    REQUIRE_FALSE(compileSource("1 2;").has_value());
}

// Strings
TEST_CASE("Concatenating two strings", "[compiler]") {
    REQUIRE(compileSource("\"hello\" + \" world\";").has_value());
}

TEST_CASE("String comparisons", "[compiler]") {
    REQUIRE(compileSource("\"hello\" == \"hello\";").has_value());
    REQUIRE(compileSource("\"apple\" > \"bannana\";").has_value());
    REQUIRE(compileSource("\"apple\" >= \"apple\";").has_value());
    REQUIRE(compileSource("\"bannana\" < \"apple\";").has_value());
    REQUIRE(compileSource("\"bannana\" <= \"bannana\";").has_value());
    REQUIRE(compileSource("\"apple\" != \"bannana\";").has_value());
}