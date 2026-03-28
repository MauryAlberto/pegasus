#pragma once
#include <cstdio>
#include <cstdint>
#include <string>
#include <string_view>
#include "chunk.hpp"
#include "scanner.hpp"

namespace pegasus {
    class Parser {
        public:
            Parser() = delete;
            explicit Parser(Scanner& scanner) : scanner_{&scanner} {}

            void advance() {
                previous_ = current_;
                while(true) {
                    current_ = scanner_->scanToken();
                    if(current_.type_ != TokenType::ERROR) break;
                    errorAtCurrent(current_.lexeme_);
                }
            }

            void consume(TokenType expectedType, std::string_view message) {
                if(current_.type_ == expectedType) {
                    advance();
                    return;
                }
                errorAtCurrent(message);
            }

            int previousLine() const            { return previous_.line_; }
            const Token& previousToken() const  { return previous_; }
            const Token& currentToken() const   { return current_; }
            bool hadError() const { return hadError_; }

        private:
            Scanner* scanner_{nullptr};
            Token current_{};
            Token previous_{};
            bool hadError_{false};
            bool panicMode_{false};

            void errorAt(const Token& token, std::string_view message) {
                if(panicMode_) return;
                panicMode_ = true;
                fprintf(stderr, "[line %d] error", token.line_);

                if(token.type_ == TokenType::EOF_) {
                    fprintf(stderr, " at end");
                } else if(token.type_ == TokenType::ERROR) {

                } else {
                    fprintf(stderr, " at '%.*s'", static_cast<int>(token.lexeme_.size()), token.lexeme_.data());
                }

                fprintf(stderr, ": %s\n", message.data());
                hadError_ = true;
            }

            void errorAtCurrent(std::string_view message) {
                errorAt(current_, message);
            }

            void error(std::string_view message) {
                errorAt(previous_, message);
            }
    };
    
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

    struct ParseRule {
        int precedence{0};
    };

    ParseRule* getRule(TokenType type) {
        static_cast<void>(type);
        return nullptr;
    }

    class Compiler {
        public:
            Compiler() = delete;
            Compiler(Parser& parser, Chunk& chunk) : parser_{&parser}, chunk_{&chunk} {}

            bool compile() {
                parser_->advance();
                expression();
                parser_->consume(TokenType::EOF_, "expect end of expression");
                endCompiler();
                return !parser_->hadError();
            }

        private:
            Parser* parser_{nullptr};
            Chunk* chunk_{nullptr};

            Chunk* currentChunk() { return chunk_; }

            void endCompiler() { emitReturn(); }

            void emitByte(OpCode op, int line = -1) {
                int actualLine{(line == -1 ? parser_->previousLine() : line)};
                currentChunk()->write(op, actualLine);
            }

            void emitByte(std::uint8_t byte, int line = -1) {
                int actualLine{(line == -1 ? parser_->previousLine() : line)};
                currentChunk()->write(byte, actualLine);
            }

            void emitBytes(std::uint8_t byte1, std::uint8_t byte2) {
                emitByte(byte1);
                emitByte(byte2);
            }

            void emitReturn() {
                emitByte(OpCode::OP_RETURN);
            }
            
            void emitConstant(Value value) {
                currentChunk()->writeConstant(value, parser_->previousLine());
            }   

            void number() {
                std::string_view lexeme{parser_->previousToken().lexeme_};
                if(lexeme.find('.') != std::string_view::npos) {
                    emitConstant(Value{std::stod(std::string{lexeme})});
                } else {
                    emitConstant(Value{std::stoi(std::string{lexeme})});
                }
            }

            void parsePrecedence(Precedence precedence) {
                static_cast<void>(precedence);
            }

            void expression() {
                parsePrecedence(Precedence::PREC_ASSIGNMENT);
            }

            void grouping() {
                expression();
                parser_->consume(TokenType::RIGHT_PAREN, "expect ')' after expression");
            }
            
            void unary() {
                TokenType operatorType{parser_->previousToken().type_};
                int line{parser_->previousLine()};

                parsePrecedence(Precedence::PREC_UNARY);

                switch(operatorType) {
                    case TokenType::MINUS: emitByte(OpCode::OP_NEGATE, line); break;
                    default: return;
                }
            }

            void binary() {
                TokenType operatorType{parser_->previousToken().type_};
                ParseRule* rule{getRule(operatorType)};
                parsePrecedence(static_cast<Precedence>(rule->precedence + 1));

                switch(operatorType) {
                    case TokenType::PLUS:   emitByte(OpCode::OP_ADD);break;
                    case TokenType::MINUS:  emitByte(OpCode::OP_SUBTRACT);break;
                    case TokenType::STAR:   emitByte(OpCode::OP_MULTIPLY);break;
                    case TokenType::SLASH:  emitByte(OpCode::OP_DIVIDE);break;
                    default: return;
                }
            }
    };

    bool compile(std::string_view source, Chunk& chunk) {
        Scanner scanner{source};
        Parser parser{scanner};
        Compiler compiler{parser, chunk};
        return compiler.compile();
    }
}