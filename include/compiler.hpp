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
#include "parser.hpp"
#include "object.hpp"
#include "function_pool.hpp"
#include "debug.hpp"

namespace pegasus {
    class Compiler {
        public:
            Compiler() = delete;
            explicit Compiler(Parser& parser, FunctionPool& funcPool) : parser_{parser}, funcPool_(funcPool) {}
            std::optional<ObjFunction> compile();

        private:
            static constexpr int DEBUG_PRINT_CODE = false;
            static constexpr int LOCAL_STACK_SIZE = 256;

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

            // State
            Parser& parser_;
            FunctionPool& funcPool_;
            ObjFunction function_{};
            FunctionType functionType_;
            Compiler* enclosing_{nullptr};

            // Upvalues
            struct UpvalueEntry {
                std::uint8_t index;
                bool isLocal; // true = captures from immediately enclosing function's locals
            };                // false = captures from further out (already an upvalue in enclosing compiler)
            std::array<UpvalueEntry, 256> upvalues_{};
            int resolveUpvalue(Compiler* compiler, const Token& name);
            int addUpvalue(std::uint8_t index, bool isLocal);

            // Locals
            static constexpr std::size_t UNINITIALIZED{std::numeric_limits<std::size_t>::max()};
            struct Local {
                std::string_view name;
                std::size_t depth;
                bool isCaptured{false};
             };
             std::array<Local, LOCAL_STACK_SIZE> locals_{};
             std::size_t localCount_{0};
             std::size_t scopeDepth_{0};

             // Parse rules
            static constexpr int TOKEN_COUNT = static_cast<int>(TokenType::TOKEN_SIZE);
            using ParseFn = void(Compiler::*)(bool canAssign);
            struct ParseRule {
                ParseFn prefix;
                ParseFn infix;
                Precedence precedence;
             };
            static const std::array<ParseRule, TOKEN_COUNT> rules;
            const ParseRule& getRule(TokenType type);

            // Bytecode emission
            Chunk* currentChunk();
            void emitByte(OpCode op, int line = -1);
            void emitByte(std::uint8_t byte, int line = -1);
            void emitReturn();
            void emitConstant(Value value);
            std::size_t emitJump(OpCode op);
            void patchJump(std::size_t offset);
            void emitLoop(std::size_t loopStart);
            void endCompiler();

            // Parsing
            void parsePrecedence(Precedence precedence);
            void declaration();
            void classDeclaration();
            void fnDeclaration();
            void varDeclaration();
            std::size_t parseVariable(std::string_view errorMessage);
            void defineVariable(std::size_t global);
            void statement();
            void synchronize();
            bool match(TokenType type);
            void printStatement();
            void ifStatement();
            void returnStatement();
            void whileStatement();
            void forStatement();
            void expressionStatement();
            void expression();
            void number(bool canAssign);
            void grouping(bool canAssign);
            void unary(bool canAssign);
            void binary(bool canAssign);
            void literal(bool canAssign);
            void string(bool canAssign);
            void dot(bool canAssign);
            void variable(bool canAssign);
            void namedVariable(const Token& type, bool canAssign);
            void and_(bool canAssign);
            void or_(bool canAssign);
            void call(bool canAssign);
            void beginScope();
            void endScope();
            void block();
            void function(FunctionType funcType);
            void declareLocalVariable();
            void addLocal(std::string_view name);
            void initializeLocal();
            int resolveLocal(const Token& name);
    };

    inline std::optional<ObjFunction> compile(std::string_view source, FunctionPool& funcPool) {
        static_cast<void>(funcPool);
        Scanner scanner{source};
        Parser parser{scanner};
        Compiler compiler{parser, funcPool};
        return compiler.compile();
    }
}