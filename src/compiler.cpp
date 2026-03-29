#include "compiler.hpp"

namespace pegasus {
    void Parser::advance() {
        previous_ = current_;
        while(true) {
            current_ = scanner_->scanToken();
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

    bool Compiler::compile() {
        parser_->advance();
        expression();
        parser_->consume(TokenType::EOF_, "expect end of expression");
        endCompiler();
        return !parser_->hadError();
    }

    Compiler::ParseRule* Compiler::getRule(TokenType type) {
        static_cast<void>(type);
        return nullptr;
    }

    Chunk* Compiler::currentChunk() { return chunk_; }

    void Compiler::endCompiler() { emitReturn(); }

    void Compiler::emitByte(OpCode op, int line) {
        int actualLine{(line == -1 ? parser_->previousLine() : line)};
        currentChunk()->write(op, actualLine);
    }

    void Compiler::emitByte(std::uint8_t byte, int line) {
        int actualLine{(line == -1 ? parser_->previousLine() : line)};
        currentChunk()->write(byte, actualLine);
    }

    void Compiler::emitBytes(std::uint8_t byte1, std::uint8_t byte2) {
        emitByte(byte1);
        emitByte(byte2);
    }

    void Compiler::emitReturn() {
        emitByte(OpCode::OP_RETURN);
    }
    
    void Compiler::emitConstant(Value value) {
        currentChunk()->writeConstant(value, parser_->previousLine());
    }   

    void Compiler::number() {
        std::string_view lexeme{parser_->previousToken().lexeme_};
        if(lexeme.find('.') != std::string_view::npos) {
            emitConstant(Value{std::stod(std::string{lexeme})});
        } else {
            emitConstant(Value{std::stoi(std::string{lexeme})});
        }
    }

    void Compiler::parsePrecedence(Precedence precedence) {
        static_cast<void>(precedence);
    }

    void Compiler::expression() {
        parsePrecedence(Precedence::PREC_ASSIGNMENT);
    }

    void Compiler::grouping() {
        expression();
        parser_->consume(TokenType::RIGHT_PAREN, "expect ')' after expression");
    }
    
    void Compiler::unary() {
        TokenType operatorType{parser_->previousToken().type_};
        int line{parser_->previousLine()};

        parsePrecedence(Precedence::PREC_UNARY);

        switch(operatorType) {
            case TokenType::MINUS: emitByte(OpCode::OP_NEGATE, line); break;
            default: return;
        }
    }

    void Compiler::binary() {
        TokenType operatorType{parser_->previousToken().type_};
        ParseRule* rule{getRule(operatorType)};
        parsePrecedence(static_cast<Precedence>(static_cast<int>(rule->precedence) + 1));

        switch(operatorType) {
            case TokenType::PLUS:   emitByte(OpCode::OP_ADD);break;
            case TokenType::MINUS:  emitByte(OpCode::OP_SUBTRACT);break;
            case TokenType::STAR:   emitByte(OpCode::OP_MULTIPLY);break;
            case TokenType::SLASH:  emitByte(OpCode::OP_DIVIDE);break;
            default: return;
        }
    }
}