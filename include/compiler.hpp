#pragma once
#include <cstdio>
#include <cstdint>
#include <array>
#include <string>
#include <limits>
#include <cstddef>
#include <optional>
#include <string_view>
#include "chunk.hpp"
#include "scanner.hpp"
#include "object.hpp"
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

    inline constexpr int LOCAL_STACK_SIZE = 256;
    class Compiler {
        public:
            Compiler() = delete;
            explicit Compiler(Parser& parser) : parser_{parser} {}
            std::optional<ObjFunction> compile();

        private:
            Parser& parser_;
            static constexpr int DEBUG_PRINT_CODE = true;

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

            enum class FunctionType {
                TYPE_FUNCTION,
                TYPE_SCRIPT
            };

            ObjFunction function_{};
            FunctionType functionType_;

            static constexpr int TOKEN_COUNT = static_cast<int>(TokenType::TOKEN_SIZE);
            using ParseFn = void(Compiler::*)(bool canAssign);

            struct ParseRule {
                ParseFn prefix;
                ParseFn infix;
                Precedence precedence;
             };

             static constexpr std::size_t UNINITIALIZED{std::numeric_limits<std::size_t>::max()};
             struct Local {
                std::string_view name;
                std::size_t depth;
             };

             std::array<Local, LOCAL_STACK_SIZE> locals_{};
             std::size_t localCount_{0};
             std::size_t scopeDepth_{0};

            const ParseRule& getRule(TokenType type);
            Chunk* currentChunk();
            void endCompiler();
            void emitByte(OpCode op, int line = -1);
            void emitByte(std::uint8_t byte, int line = -1);
            void emitReturn();
            void emitConstant(Value value);
            std::size_t emitJump(OpCode op);
            void patchJump(std::size_t offset);
            void parsePrecedence(Precedence precedence);
            void declaration();
            void varDeclaration();
            std::size_t parseVariable(std::string_view errorMessage);
            void defineVariable(std::size_t global);
            void statement();
            void synchronize();
            bool match(TokenType type);
            void printStatement();
            void ifStatement();
            void whileStatement();
            void forStatement();
            void emitLoop(std::size_t loopStart);
            void expressionStatement();
            void expression();
            void number(bool canAssign);
            void grouping(bool canAssign);
            void unary(bool canAssign);
            void binary(bool canAssign);
            void literal(bool canAssign);
            void string(bool canAssign);
            void variable(bool canAssign);
            void namedVariable(const Token& type, bool canAssign);
            void and_(bool canAssign);
            void or_(bool canAssign);
            void beginScope();
            void endScope();
            void block();
            void declareVariable();
            void addLocal(std::string_view name);
            void markInitialized();
            int resolveLocal(const Token& name);

            static const std::array<ParseRule, TOKEN_COUNT> rules;
    };

    inline std::optional<ObjFunction> compile(std::string_view source) {
        Scanner scanner{source};
        Parser parser{scanner};
        Compiler compiler{parser};
        return compiler.compile();
    }
}