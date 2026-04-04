#pragma once
#include <cstdio>
#include <cstdint>
#include <array>
#include <string>
#include <cstddef>
#include <string_view>
#include "chunk.hpp"
#include "scanner.hpp"
#include "debug.hpp"

namespace pegasus {
    class Parser {
        public:
            Parser() = delete;
            explicit Parser(Scanner& scanner) : scanner_{scanner} {}

            void advance();
            void consume(TokenType expectedType, std::string_view message);
            int previousLine() const;
            const Token& previousToken() const;
            const Token& currentToken() const;
            bool hadError() const;
            bool isInPanicMode() const;
            void resetPanicMode();
            void errorAt(const Token& token, std::string_view message);
            void errorAtCurrent(std::string_view message);
            void error(std::string_view message);

        private:
            Scanner& scanner_;
            Token current_{};
            Token previous_{};
            bool hadError_{false};
            bool panicMode_{false};
    };

    class Compiler {
        public:
            Compiler() = delete;
            Compiler(Parser& parser, Chunk& chunk) : parser_{parser}, chunk_{&chunk} {}
            bool compile();

        private:
            Parser& parser_;
            Chunk* chunk_{nullptr};
            static constexpr int DEBUG_PRINT_CODE = false;

            enum class Precedence {
                PREC_NONE,
                PREC_ASSIGNMENT,    // =
                PREC_OR,            // or
                PREC_AND,           // and
                PREC_EQUALITY,      // == !=
                PREC_COMPARISON,    // < > <= >=
                PREC_TERM,          // + -
                PREC_FACTOR,        // * /
                PREC_UNARY,         // ! -
                PREC_CALL,          // . ()
                PREC_PRIMARY
            };

            static constexpr int TOKEN_COUNT = static_cast<int>(TokenType::TOKEN_SIZE);
            static_assert(TOKEN_COUNT == 40, "token count mismatch");
            using ParseFn = void(Compiler::*)();

            struct ParseRule {
                ParseFn prefix;
                ParseFn infix;
                Precedence precedence;
             };

            const ParseRule& getRule(TokenType type);
            Chunk* currentChunk();
            void endCompiler();
            void emitByte(OpCode op, int line = -1);
            void emitByte(std::uint8_t byte, int line = -1);
            void emitReturn();
            void emitConstant(Value value);
            void parsePrecedence(Precedence precedence);
            void declaration();
            void varDeclaration();
            std::size_t parseVariable(std::string_view errorMessage);
            void defineVariable(std::size_t global);
            void statement();
            void synchronize();
            bool match(TokenType type);
            void printStatement();
            void expressionStatement();
            void number();
            void expression();
            void grouping();
            void unary();
            void binary();
            void literal();
            void string();
            void variable();
            void namedVariable(const Token& type);

            static const std::array<ParseRule, TOKEN_COUNT> rules;
    };

    inline bool compile(std::string_view source, Chunk& chunk) {
        Scanner scanner{source};
        Parser parser{scanner};
        Compiler compiler{parser, chunk};
        return compiler.compile();
    }
}