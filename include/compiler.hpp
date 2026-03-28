#pragma once
#include <cstdio>
#include <cstdint>
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

            void consume(TokenType type, std::string_view message) {
                if(current_.type_ == type) {
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
                fprintf(stderr, "[line %d] error ", token.line_);

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

    class Compiler {
        public:
            Compiler() = delete;
            Compiler(Parser& parser, Chunk& chunk) : parser_{&parser}, chunk_{&chunk} {}

            void emitByte(OpCode op) {
                chunk_->write(op, parser_->previousLine());
            }

            void emitByte(std::uint8_t byte) {
                chunk_->write(byte, parser_->previousLine());
            }

            void emitBytes(std::uint8_t byte1, std::uint8_t byte2) {
                emitByte(byte1);
                emitByte(byte2);
            }

            void emitReturn() {
                emitByte(OpCode::OP_RETURN);
            }

            void endCompiler() {
                emitReturn();
            }

            void expression() {
                
            }

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
    };

    bool compile(std::string_view source, Chunk& chunk) {
        Scanner scanner{source};
        Parser parser{scanner};
        Compiler compiler{parser, chunk};
        return compiler.compile();
    }
}