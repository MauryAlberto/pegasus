#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "vm.hpp"

using namespace pegasus;

// Helper
static InterpretResult interpretSource(const std::string& source) {
    VM vm;
    return vm.interpret(source);
}

// Full pipeline: interpret() from source string
TEST_CASE("interpret() returns OK for a valid expression", "[vm]") {
    REQUIRE(interpretSource("1 + 2;") == InterpretResult::OK);
}

TEST_CASE("interpret() returns COMPILE_ERROR for bad syntax", "[vm]") {
    REQUIRE(interpretSource("(1 +;") == InterpretResult::COMPILE_ERROR);
}

TEST_CASE("interpret() returns OK for complex expression", "[vm]") {
    REQUIRE(interpretSource("(1 + 2) * 3 - 4 / 2;") == InterpretResult::OK);
}

TEST_CASE("interpret() handles unary negation", "[vm]") {
    REQUIRE(interpretSource("-42;") == InterpretResult::OK);
}

TEST_CASE("interpret() handles nested parens", "[vm]") {
    REQUIRE(interpretSource("((10));") == InterpretResult::OK);
}

TEST_CASE("interpret() handles double negation", "[vm]") {
    REQUIRE(interpretSource("--5;") == InterpretResult::OK);
}

TEST_CASE("interpret() returns OK for float arithmetic", "[vm]") {
    REQUIRE(interpretSource("1.5 + 2.5;") == InterpretResult::OK);
}

TEST_CASE("interpret() returns OK for mixed int/float expression", "[vm]") {
    REQUIRE(interpretSource("10 / 3.0;") == InterpretResult::OK);
}

TEST_CASE("interpret returns OK for complex comparison logic", "[vm]") {
    REQUIRE(interpretSource("!(5 - 4 > 3 * 2 == !nil);") == InterpretResult::OK);
}

TEST_CASE("interpret returns OK for string contatenation", "[vm]") {
    REQUIRE(interpretSource("\"hello\" + \" world\";") == InterpretResult::OK);
}

TEST_CASE("interpret returns OK for string comparions", "[vm]") {
    REQUIRE(interpretSource("\"hello\" == \"hello\";") == InterpretResult::OK);
    REQUIRE(interpretSource("\"apple\" > \"bannana\";") == InterpretResult::OK);
    REQUIRE(interpretSource("\"bannana\" < \"apple\";") == InterpretResult::OK);
    REQUIRE(interpretSource("\"bannana\" <= \"bannana\";") == InterpretResult::OK);
}

TEST_CASE("interpret() returns OK for a bare number", "[vm]") {
    REQUIRE(interpretSource("99;") == InterpretResult::OK);
}