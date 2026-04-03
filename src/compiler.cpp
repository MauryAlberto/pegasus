#include "compiler.hpp"

namespace pegasus {
    

    const std::array<Compiler::ParseRule, Compiler::TOKEN_COUNT> Compiler::rules {{

        /* LEFT_PAREN */    {&Compiler::grouping, nullptr, Compiler::Precedence::PREC_NONE},
        /* RIGHT_PAREN */   {nullptr, nullptr, Compiler::Precedence::PREC_NONE},
        /* LEFT_BRACE */    {nullptr, nullptr, Compiler::Precedence::PREC_NONE},
        /* RIGHT_BRACE */   {nullptr, nullptr, Compiler::Precedence::PREC_NONE},
        /* COMMA */         {nullptr, nullptr, Compiler::Precedence::PREC_NONE},
        /* DOT */           {nullptr, nullptr, Compiler::Precedence::PREC_NONE},
        /* MINUS */         {&Compiler::unary, &Compiler::binary, Compiler::Precedence::PREC_TERM},
        /* PLUS */          {nullptr, &Compiler::binary, Compiler::Precedence::PREC_TERM},
        /* SEMICOLON */     {nullptr, nullptr, Compiler::Precedence::PREC_NONE},
        /* SLASH */         {nullptr, &Compiler::binary, Compiler::Precedence::PREC_FACTOR},
        /* STAR */          {nullptr, &Compiler::binary, Compiler::Precedence::PREC_FACTOR},
        // one or two character tokens
        /* NOT */           {&Compiler::unary, nullptr, Compiler::Precedence::PREC_NONE},
        /* NOT_EQUAL */     {nullptr, &Compiler::binary, Compiler::Precedence::PREC_EQUALITY},
        /* EQUAL */         {nullptr, nullptr, Compiler::Precedence::PREC_NONE},
        /* EQUAL_EQUAL */   {nullptr, &Compiler::binary, Compiler::Precedence::PREC_EQUALITY},
        /* GREATER */       {nullptr, &Compiler::binary, Compiler::Precedence::PREC_COMPARISON},
        /* GREATER_EQUAL */ {nullptr, &Compiler::binary, Compiler::Precedence::PREC_COMPARISON},
        /* LESS */          {nullptr, &Compiler::binary, Compiler::Precedence::PREC_COMPARISON},
        /* LESS_EQUAL */    {nullptr, &Compiler::binary, Compiler::Precedence::PREC_COMPARISON},
        // literals
        /* IDENTIFIER */    {nullptr, nullptr, Compiler::Precedence::PREC_NONE},
        /* STRING */        {&Compiler::string, nullptr, Compiler::Precedence::PREC_NONE},
        /* NUMBER */        {&Compiler::number, nullptr, Compiler::Precedence::PREC_NONE},
        // keywords
        /* TRUE */          {&Compiler::literal, nullptr, Compiler::Precedence::PREC_NONE},
        /* FALSE */         {&Compiler::literal, nullptr, Compiler::Precedence::PREC_NONE},
        /* IF */            {nullptr, nullptr, Compiler::Precedence::PREC_NONE},
        /* ELSE */          {nullptr, nullptr, Compiler::Precedence::PREC_NONE},
        /* AND */           {nullptr, nullptr, Compiler::Precedence::PREC_NONE},
        /* OR */            {nullptr, nullptr, Compiler::Precedence::PREC_NONE},
        /* FOR */           {nullptr, nullptr, Compiler::Precedence::PREC_NONE},
        /* WHILE */         {nullptr, nullptr, Compiler::Precedence::PREC_NONE},
        /* FUN */           {nullptr, nullptr, Compiler::Precedence::PREC_NONE},
        /* CLASS */         {nullptr, nullptr, Compiler::Precedence::PREC_NONE},
        /* PRINT */         {nullptr, nullptr, Compiler::Precedence::PREC_NONE},
        /* RETURN */        {nullptr, nullptr, Compiler::Precedence::PREC_NONE},
        /* VAR */           {nullptr, nullptr, Compiler::Precedence::PREC_NONE},
        /* THIS */          {nullptr, nullptr, Compiler::Precedence::PREC_NONE},
        /* NIL */           {&Compiler::literal, nullptr, Compiler::Precedence::PREC_NONE},
        /* SUPER */         {nullptr, nullptr, Compiler::Precedence::PREC_NONE},

        /* ERROR */         {nullptr, nullptr, Compiler::Precedence::PREC_NONE},
         /* EOF_ */         {nullptr, nullptr, Compiler::Precedence::PREC_NONE}
    }};


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
        parser_.advance();
        expression();
        parser_.consume(TokenType::EOF_, "expect end of expression");
        endCompiler();
        return !parser_.hadError();
    }

    const Compiler::ParseRule& Compiler::getRule(TokenType type) {
        return rules[static_cast<std::size_t>(type)];
    }

    Chunk* Compiler::currentChunk() { return chunk_; }

    void Compiler::endCompiler() { 
        emitReturn();
        if(DEBUG_PRINT_CODE && !parser_.hadError()) {
            disassembleChunk(currentChunk(), "code");
        }
    }

    void Compiler::emitByte(OpCode op, int line) {
        int actualLine{(line == -1 ? parser_.previousLine() : line)};
        currentChunk()->write(op, actualLine);
    }

    void Compiler::emitByte(std::uint8_t byte, int line) {
        int actualLine{(line == -1 ? parser_.previousLine() : line)};
        currentChunk()->write(byte, actualLine);
    }

    void Compiler::emitReturn() {
        emitByte(OpCode::OP_RETURN);
    }
    
    void Compiler::emitConstant(Value value) {
        currentChunk()->writeConstant(value, parser_.previousLine());
    }   

    void Compiler::number() {
        std::string_view lexeme{parser_.previousToken().lexeme_};
        if(lexeme.find('.') != std::string_view::npos) {
            emitConstant(Value{std::stod(std::string{lexeme})});
        } else {
            emitConstant(Value{std::stoi(std::string{lexeme})});
        }
    }

    void Compiler::parsePrecedence(Precedence precedence) {
        parser_.advance();
        ParseFn prefixRule{getRule(parser_.previousToken().type_).prefix};
        
        if(prefixRule == nullptr) {
            parser_.error("expected expression");
            return;
        }

        (this->*prefixRule)();

        while(precedence <= getRule(parser_.currentToken().type_).precedence) {
            parser_.advance();
            ParseFn infixRule{getRule(parser_.previousToken().type_).infix};
            (this->*infixRule)();
        }
    }

    void Compiler::expression() {
        parsePrecedence(Precedence::PREC_ASSIGNMENT);
    }

    void Compiler::grouping() {
        expression();
        parser_.consume(TokenType::RIGHT_PAREN, "expect ')' after expression");
    }
    
    void Compiler::unary() {
        TokenType operatorType{parser_.previousToken().type_};
        int line{parser_.previousLine()};

        parsePrecedence(Precedence::PREC_UNARY);

        switch(operatorType) {
            case TokenType::MINUS:  emitByte(OpCode::OP_NEGATE, line);break;
            case TokenType::NOT:    emitByte(OpCode::OP_NOT, line);break;
            default: return;
        }
    }

    void Compiler::binary() {
        TokenType operatorType{parser_.previousToken().type_};
        const ParseRule& rule{getRule(operatorType)};
        parsePrecedence(static_cast<Precedence>(static_cast<std::size_t>(rule.precedence) + 1));

        switch(operatorType) {
            case TokenType::PLUS:           emitByte(OpCode::OP_ADD);break;
            case TokenType::MINUS:          emitByte(OpCode::OP_SUBTRACT);break;
            case TokenType::STAR:           emitByte(OpCode::OP_MULTIPLY);break;
            case TokenType::SLASH:          emitByte(OpCode::OP_DIVIDE);break;
            case TokenType::NOT:            emitByte(OpCode::OP_NOT);break;
            case TokenType::NOT_EQUAL:      emitByte(OpCode::OP_EQUAL);emitByte(OpCode::OP_NOT);break;
            case TokenType::EQUAL_EQUAL:    emitByte(OpCode::OP_EQUAL);break;
            case TokenType::GREATER:        emitByte(OpCode::OP_GREATER);break;
            case TokenType::GREATER_EQUAL:  emitByte(OpCode::OP_LESS);emitByte(OpCode::OP_NOT);break;
            case TokenType::LESS:           emitByte(OpCode::OP_LESS);break;
            case TokenType::LESS_EQUAL:     emitByte(OpCode::OP_GREATER);emitByte(OpCode::OP_NOT);break;
            default: return;
        }
    }

    void Compiler::literal() {
        switch(parser_.previousToken().type_) {
            case TokenType::TRUE:   emitByte(OpCode::OP_TRUE);break;
            case TokenType::FALSE:  emitByte(OpCode::OP_FALSE);break;
            case TokenType::NIL:    emitByte(OpCode::OP_NIL);break;
            default: return;
        }
    }

    void Compiler::string() {
        emitConstant(Value{parser_.previousToken().lexeme_});
    }
}