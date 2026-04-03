#include <catch2/catch_test_macros.hpp>
#include "scanner.hpp"

using namespace pegasus;

// Helper: scan all tokens from source until EOF_
static std::vector<Token> scanAll(std::string_view source) {
    // source must outlive the scanner, caller owns the string
    Scanner scanner{source};
    std::vector<Token> tokens;
    while (true) {
        Token t = scanner.scanToken();
        tokens.push_back(t);
        if (t.type_ == TokenType::EOF_) break;
    }
    return tokens;
}

// Single-character tokens
TEST_CASE("Scanner produces single-character tokens", "[scanner]") {
    std::string src = "( ) { } , . - + ; / *";
    auto tokens = scanAll(src);

    REQUIRE(tokens.size() >= 12); // 11 tokens + EOF
    REQUIRE(tokens[0].type_ == TokenType::LEFT_PAREN);
    REQUIRE(tokens[1].type_ == TokenType::RIGHT_PAREN);
    REQUIRE(tokens[2].type_ == TokenType::LEFT_BRACE);
    REQUIRE(tokens[3].type_ == TokenType::RIGHT_BRACE);
    REQUIRE(tokens[4].type_ == TokenType::COMMA);
    REQUIRE(tokens[5].type_ == TokenType::DOT);
    REQUIRE(tokens[6].type_ == TokenType::MINUS);
    REQUIRE(tokens[7].type_ == TokenType::PLUS);
    REQUIRE(tokens[8].type_ == TokenType::SEMICOLON);
    REQUIRE(tokens[9].type_ == TokenType::SLASH);
    REQUIRE(tokens[10].type_ == TokenType::STAR);
    REQUIRE(tokens[11].type_ == TokenType::EOF_);
}

// Two-character tokens
TEST_CASE("Scanner produces two-character tokens", "[scanner]") {
    std::string src = "! != = == < <= > >=";
    auto tokens = scanAll(src);

    REQUIRE(tokens[0].type_ == TokenType::NOT);
    REQUIRE(tokens[1].type_ == TokenType::NOT_EQUAL);
    REQUIRE(tokens[2].type_ == TokenType::EQUAL);
    REQUIRE(tokens[3].type_ == TokenType::EQUAL_EQUAL);
    REQUIRE(tokens[4].type_ == TokenType::LESS);
    REQUIRE(tokens[5].type_ == TokenType::LESS_EQUAL);
    REQUIRE(tokens[6].type_ == TokenType::GREATER);
    REQUIRE(tokens[7].type_ == TokenType::GREATER_EQUAL);
}

// Number literals
TEST_CASE("Scanner scans integer literals", "[scanner]") {
    std::string src = "42";
    auto tokens = scanAll(src);
    REQUIRE(tokens[0].type_ == TokenType::NUMBER);
    REQUIRE(tokens[0].lexeme_ == "42");
}

TEST_CASE("Scanner scans floating-point literals", "[scanner]") {
    std::string src = "3.14";
    auto tokens = scanAll(src);
    REQUIRE(tokens[0].type_ == TokenType::NUMBER);
    REQUIRE(tokens[0].lexeme_ == "3.14");
}

TEST_CASE("Trailing dot is not consumed as part of number", "[scanner]") {
    // "123." — the dot is separate because there's no digit after it
    std::string src = "123.";
    auto tokens = scanAll(src);
    REQUIRE(tokens[0].type_ == TokenType::NUMBER);
    REQUIRE(tokens[0].lexeme_ == "123");
    REQUIRE(tokens[1].type_ == TokenType::DOT);
}

// String literals
TEST_CASE("Scanner scans a simple string", "[scanner]") {
    std::string src = "\"hello world\"";
    auto tokens = scanAll(src);
    REQUIRE(tokens[0].type_ == TokenType::STRING);
    REQUIRE(tokens[0].lexeme_ == "hello world");
}

TEST_CASE("Unterminated string produces ERROR token", "[scanner]") {
    std::string src = "\"oops";
    auto tokens = scanAll(src);
    REQUIRE(tokens[0].type_ == TokenType::ERROR);
}

TEST_CASE("Multi-line string tracks line numbers", "[scanner]") {
    std::string src = "\"line1\nline2\"";
    auto tokens = scanAll(src);
    REQUIRE(tokens[0].type_ == TokenType::STRING);
    // After the string, the scanner should have advanced the line counter
    REQUIRE(tokens[1].type_ == TokenType::EOF_);
    REQUIRE(tokens[1].line_ == 2);
}

// Keywords
TEST_CASE("Scanner recognises all keywords", "[scanner]") {
    std::string src =
        "and class else false for fun if nil or print "
        "return super this true var while";
    auto tokens = scanAll(src);

    REQUIRE(tokens[0].type_  == TokenType::AND);
    REQUIRE(tokens[1].type_  == TokenType::CLASS);
    REQUIRE(tokens[2].type_  == TokenType::ELSE);
    REQUIRE(tokens[3].type_  == TokenType::FALSE);
    REQUIRE(tokens[4].type_  == TokenType::FOR);
    REQUIRE(tokens[5].type_  == TokenType::FUN);
    REQUIRE(tokens[6].type_  == TokenType::IF);
    REQUIRE(tokens[7].type_  == TokenType::NIL);
    REQUIRE(tokens[8].type_  == TokenType::OR);
    REQUIRE(tokens[9].type_  == TokenType::PRINT);
    REQUIRE(tokens[10].type_ == TokenType::RETURN);
    REQUIRE(tokens[11].type_ == TokenType::SUPER);
    REQUIRE(tokens[12].type_ == TokenType::THIS);
    REQUIRE(tokens[13].type_ == TokenType::TRUE);
    REQUIRE(tokens[14].type_ == TokenType::VAR);
    REQUIRE(tokens[15].type_ == TokenType::WHILE);
}

TEST_CASE("Identifiers that start like keywords stay IDENTIFIER", "[scanner]") {
    std::string src = "android classify format";
    auto tokens = scanAll(src);
    REQUIRE(tokens[0].type_ == TokenType::IDENTIFIER);
    REQUIRE(tokens[1].type_ == TokenType::IDENTIFIER);
    REQUIRE(tokens[2].type_ == TokenType::IDENTIFIER);
}

// Identifiers
TEST_CASE("Scanner scans identifiers with underscores and digits", "[scanner]") {
    std::string src = "_foo bar123 _baz_42";
    auto tokens = scanAll(src);
    REQUIRE(tokens[0].type_ == TokenType::IDENTIFIER);
    REQUIRE(tokens[0].lexeme_ == "_foo");
    REQUIRE(tokens[1].type_ == TokenType::IDENTIFIER);
    REQUIRE(tokens[1].lexeme_ == "bar123");
    REQUIRE(tokens[2].type_ == TokenType::IDENTIFIER);
    REQUIRE(tokens[2].lexeme_ == "_baz_42");
}

// Whitespace & comments
TEST_CASE("Scanner skips whitespace", "[scanner]") {
    std::string src = "  \t\r 42";
    auto tokens = scanAll(src);
    REQUIRE(tokens[0].type_ == TokenType::NUMBER);
    REQUIRE(tokens[0].lexeme_ == "42");
}

TEST_CASE("Scanner skips line comments", "[scanner]") {
    std::string src = "// this is a comment\n42";
    auto tokens = scanAll(src);
    REQUIRE(tokens[0].type_ == TokenType::NUMBER);
    REQUIRE(tokens[0].lexeme_ == "42");
    REQUIRE(tokens[0].line_ == 2);
}

TEST_CASE("Scanner tracks line numbers across newlines", "[scanner]") {
    std::string src = "(\n)\n+";
    auto tokens = scanAll(src);
    REQUIRE(tokens[0].line_ == 1);
    REQUIRE(tokens[1].line_ == 2);
    REQUIRE(tokens[2].line_ == 3);
}

// Edge cases
TEST_CASE("Empty source yields only EOF", "[scanner]") {
    std::string src = "";
    auto tokens = scanAll(src);
    REQUIRE(tokens.size() == 1);
    REQUIRE(tokens[0].type_ == TokenType::EOF_);
}

TEST_CASE("Unexpected character produces ERROR token", "[scanner]") {
    std::string src = "@";
    auto tokens = scanAll(src);
    REQUIRE(tokens[0].type_ == TokenType::ERROR);
}
