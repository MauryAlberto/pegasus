#include "parser.hpp"

namespace pegasus {
    void Parser::advance() {
        previous_ = current_;
        while(true) {
            current_ = scanner_.scanToken();
            if(current_.type_ != TokenType::ERROR) break;
            errorAtCurrent(current_.lexeme_);
        }
    }

    void Parser::consume(TokenType expectedType, std::string_view message) {
        if(current_.type_ == expectedType) {
            advance();
            return;
        }
        errorAtCurrent(message);
    }

    int Parser::previousLine() const            { return previous_.line_; }

    const Token& Parser::previousToken() const  { return previous_; }

    const Token& Parser::currentToken() const   { return current_; }

    bool Parser::hadError() const { return hadError_; }

    bool Parser::isInPanicMode() const {
        return panicMode_;
    }

    void Parser::resetPanicMode() {
        panicMode_ = false;
    }

    void Parser::errorAt(const Token& token, std::string_view message) {
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

    void Parser::errorAtCurrent(std::string_view message) {
        errorAt(current_, message);
    }

    void Parser::error(std::string_view message) {
        errorAt(previous_, message);
    }
}