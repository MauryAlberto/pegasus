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
        /* IDENTIFIER */    {&Compiler::variable, nullptr, Compiler::Precedence::PREC_NONE},
        /* STRING */        {&Compiler::string, nullptr, Compiler::Precedence::PREC_NONE},
        /* NUMBER */        {&Compiler::number, nullptr, Compiler::Precedence::PREC_NONE},
        // keywords
        /* TRUE */          {&Compiler::literal, nullptr, Compiler::Precedence::PREC_NONE},
        /* FALSE */         {&Compiler::literal, nullptr, Compiler::Precedence::PREC_NONE},
        /* IF */            {nullptr, nullptr, Compiler::Precedence::PREC_NONE},
        /* ELSE */          {nullptr, nullptr, Compiler::Precedence::PREC_NONE},
        /* AND */           {nullptr, &Compiler::and_, Compiler::Precedence::PREC_NONE},
        /* OR */            {nullptr, &Compiler::or_, Compiler::Precedence::PREC_NONE},
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

    bool Compiler::compile() {
        parser_.advance();
        while(!match(TokenType::EOF_)) declaration();
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

    std::size_t Compiler::emitJump(OpCode op) {
        emitByte(op);
        emitByte(0xFF);
        emitByte(0xFF);
        return currentChunk()->getCodeSize() - 2;
    }

    void Compiler::patchJump(std::size_t jumpOperandIndex) {
        std::size_t jump{currentChunk()->getCodeSize() - jumpOperandIndex - 2};

        if(jump > std::numeric_limits<std::uint16_t>::max()) {
            parser_.error("too much code to jump over");
        }

        currentChunk()->setByte(jumpOperandIndex, (jump & 0xFF));
        currentChunk()->setByte(jumpOperandIndex + 1, ((jump >> 8)) & 0xFF);
    }

    void Compiler::parsePrecedence(Precedence precedence) {
        parser_.advance();
        ParseFn prefixRule{getRule(parser_.previousToken().type_).prefix};
        
        if(prefixRule == nullptr) {
            parser_.error("expected expression");
            return;
        }

        bool canAssign{precedence <= Precedence::PREC_ASSIGNMENT};
        (this->*prefixRule)(canAssign);

        while(precedence <= getRule(parser_.currentToken().type_).precedence) {
            parser_.advance();
            ParseFn infixRule{getRule(parser_.previousToken().type_).infix};
            (this->*infixRule)(canAssign);
        }

        if(canAssign && match(TokenType::EQUAL)) {
            parser_.error("invalid assignment target");
        }
    }

    void Compiler::declaration() {
        if(match(TokenType::VAR)) {
            varDeclaration();
        } else {
            statement();
        }

        if(parser_.isInPanicMode()) synchronize();
    }

    void Compiler::varDeclaration() {
        std::size_t global{parseVariable("expect variable name")};

        if(match(TokenType::EQUAL)) {
            expression();
        } else {
            emitByte(OpCode::OP_NIL);
        }

        parser_.consume(TokenType::SEMICOLON, "expected ';' after variable declaration");
        defineVariable(global);
    }

    std::size_t Compiler::parseVariable(std::string_view errorMessage) {
        parser_.consume(TokenType::IDENTIFIER, errorMessage);

        if(scopeDepth_ > 0) {
            declareVariable();
            return 0;
        }

        return chunk_->addConstant(Value{parser_.previousToken().lexeme_});
    }

    void Compiler::defineVariable(const std::size_t global) {
        if(scopeDepth_ > 0) {
            markInitialized();
            return;
        }

        if(global <= 255) {
            emitByte(OpCode::OP_DEFINE_GLOBAL);
            emitByte(static_cast<std::uint8_t>(global));
        } else {
            emitByte(OpCode::OP_DEFINE_GLOBAL_LONG);
            emitByte(static_cast<std::uint8_t>(global & 0xFF));
            emitByte(static_cast<std::uint8_t>((global >> 8) & 0xFF));
            emitByte(static_cast<std::uint8_t>((global >> 16) & 0xFF));
        }
    }

    void Compiler::statement() {
        if(match(TokenType::PRINT)) {
            printStatement();
        } else if(match(TokenType::IF)) {
            ifStatement();
        } else if(match(TokenType::WHILE)) {
            whileStatement();
        } else if(match(TokenType::FOR)) {
            forStatement();
        } else if(match(TokenType::LEFT_BRACE)) {
            beginScope();
            block();
            endScope();
        } else {
            expressionStatement();
        }
    }

    void Compiler::synchronize() {
        parser_.resetPanicMode();

        while(parser_.currentToken().type_ != TokenType::EOF_) {
            if(parser_.previousToken().type_ == TokenType::SEMICOLON) return;
            switch(parser_.currentToken().type_) {
                case TokenType::CLASS:
                case TokenType::FUN:
                case TokenType::VAR:
                case TokenType::FOR:
                case TokenType::IF:
                case TokenType::WHILE:
                case TokenType::PRINT:
                case TokenType::RETURN:
                    return;
                default:;
            }

            parser_.advance();
        }
    }

    bool Compiler::match(TokenType type) {
        if(parser_.currentToken().type_ != type) return false;
        parser_.advance();
        return true;
    }

    void Compiler::printStatement() {
        expression();
        parser_.consume(TokenType::SEMICOLON, "expected ';' after value");
        emitByte(OpCode::OP_PRINT);
    }

    void Compiler::ifStatement() {
        parser_.consume(TokenType::LEFT_PAREN, "expected '(' after if");
        expression();
        parser_.consume(TokenType::RIGHT_PAREN, "expected ')' after condition");

        std::size_t jumpThenBlock{emitJump(OpCode::OP_JUMP_IF_FALSE)};
        emitByte(OpCode::OP_POP);   // if not false then pop value off the stack from expression
        statement();                // execute the statement(s)
        std::size_t jumpElseBlock(emitJump(OpCode::OP_JUMP)); // jump past the else block
        
        patchJump(jumpThenBlock); // now that we have the code size for the "then" block we can figure out how much to jump by

        emitByte(OpCode::OP_POP);   // if false we jump here and pop value off the stack from expression
        if(match(TokenType::ELSE)) statement(); // execute statment(s)
        patchJump(jumpElseBlock); // now that we have the code size for the "else" block we can figure out how much to jump by
    }

    void Compiler::whileStatement() {
        std::size_t loopStart{currentChunk()->getCodeSize()};
        parser_.consume(TokenType::LEFT_PAREN, "expect '(' after 'while'");
        expression();
        parser_.consume(TokenType::RIGHT_PAREN, "expect ')' after condition");

        std::size_t exitJump{emitJump(OpCode::OP_JUMP_IF_FALSE)};
        emitByte(OpCode::OP_POP);
        statement();
        emitLoop(loopStart);

        patchJump(exitJump);
        emitByte(OpCode::OP_POP);
    }

    void Compiler::forStatement() {
        beginScope();
        parser_.consume(TokenType::LEFT_PAREN, "expect '(' after 'for'");
        if(match(TokenType::SEMICOLON)) {
            // no initializer
        } else if(match(TokenType::VAR)) {
            varDeclaration();
        } else {
            expressionStatement();
        }

        std::size_t loopStart{currentChunk()->getCodeSize()};
        std::size_t exitJump{0};
        if(!match(TokenType::SEMICOLON)) {
            expression();
            parser_.consume(TokenType::SEMICOLON, "expect ';' after loop condition");

            exitJump = emitJump(OpCode::OP_JUMP_IF_FALSE);
            emitByte(OpCode::OP_POP);
        }

        if(!match(TokenType::RIGHT_PAREN)) {
            std::size_t bodyJump{emitJump(OpCode::OP_JUMP)};
            std::size_t incrementStart{currentChunk()->getCodeSize()};
            expression();
            emitByte(OpCode::OP_POP);

            parser_.consume(TokenType::RIGHT_PAREN, "expect ')' after for clauses");
            emitLoop(loopStart);
            loopStart = incrementStart;
            patchJump(bodyJump);
        }

        statement();
        emitLoop(loopStart);

        if(exitJump != 0) {
            patchJump(exitJump);
            emitByte(OpCode::OP_POP);
        }

        endScope();
    }

    void Compiler::emitLoop(std::size_t loopStart) {
        emitByte(OpCode::OP_LOOP);

        std::size_t offset{currentChunk()->getCodeSize() - loopStart + 2};
        if(offset > std::numeric_limits<std::uint16_t>::max()) {
            parser_.error("loop body too large");
        }

        emitByte(static_cast<std::uint8_t>(offset & 0xFF));
        emitByte(static_cast<std::uint8_t>((offset >> 8) & 0xFF));
    }

    void Compiler::expressionStatement() {
        expression();
        parser_.consume(TokenType::SEMICOLON, "expected ';' after expression");
        emitByte(OpCode::OP_POP);
    }

    void Compiler::expression() {
        parsePrecedence(Precedence::PREC_ASSIGNMENT);
    }

    void Compiler::number(bool canAssign) {
        static_cast<void>(canAssign);
        std::string_view lexeme{parser_.previousToken().lexeme_};
        if(lexeme.find('.') != std::string_view::npos) {
            emitConstant(Value{std::stod(std::string{lexeme})});
        } else {
            emitConstant(Value{std::stoi(std::string{lexeme})});
        }
    }

    void Compiler::grouping(bool canAssign) {
        static_cast<void>(canAssign);
        expression();
        parser_.consume(TokenType::RIGHT_PAREN, "expect ')' after expression");
    }
    
    void Compiler::unary(bool canAssign) {
        static_cast<void>(canAssign);
        TokenType operatorType{parser_.previousToken().type_};
        int line{parser_.previousLine()};

        parsePrecedence(Precedence::PREC_UNARY);

        switch(operatorType) {
            case TokenType::MINUS:  emitByte(OpCode::OP_NEGATE, line);break;
            case TokenType::NOT:    emitByte(OpCode::OP_NOT, line);break;
            default: return;
        }
    }

    void Compiler::binary(bool canAssign) {
        static_cast<void>(canAssign);
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

    void Compiler::literal(bool canAssign) {
        static_cast<void>(canAssign);
        switch(parser_.previousToken().type_) {
            case TokenType::TRUE:   emitByte(OpCode::OP_TRUE);break;
            case TokenType::FALSE:  emitByte(OpCode::OP_FALSE);break;
            case TokenType::NIL:    emitByte(OpCode::OP_NIL);break;
            default: return;
        }
    }

    void Compiler::string(bool canAssign) {
        static_cast<void>(canAssign);
        emitConstant(Value{std::string(parser_.previousToken().lexeme_)});
    }
    
    void Compiler::variable(bool canAssign) {
        namedVariable(parser_.previousToken(), canAssign);
    }

    void Compiler::namedVariable(const Token& name, bool canAssign) {
        static_cast<void>(canAssign);
        int arg{resolveLocal(name)};
        bool isLocal{arg != -1};

        if(!isLocal) {
            std::size_t globalArg{chunk_->addConstant(Value{name.lexeme_})};

            if(canAssign && match(TokenType::EQUAL)) {
                expression();
                if(globalArg <= 255) {
                    emitByte(OpCode::OP_SET_GLOBAL);
                    emitByte(static_cast<std::uint8_t>(globalArg));
                } else {
                    emitByte(OpCode::OP_SET_GLOBAL_LONG);
                    emitByte(static_cast<std::uint8_t>(globalArg & 0xFF));
                    emitByte(static_cast<std::uint8_t>((globalArg >> 8) & 0xFF));
                    emitByte(static_cast<std::uint8_t>((globalArg >> 16) & 0xFF));
                }
            } else {
                if(globalArg <= 255) {
                    emitByte(OpCode::OP_GET_GLOBAL);
                    emitByte(static_cast<std::uint8_t>(globalArg));
                } else {
                    emitByte(OpCode::OP_GET_GLOBAL_LONG);
                    emitByte(static_cast<std::uint8_t>(globalArg & 0xFF));
                    emitByte(static_cast<std::uint8_t>((globalArg >> 8) & 0xFF));
                    emitByte(static_cast<std::uint8_t>((globalArg >> 16) & 0xFF));
                }
            }
        } else {
            if(canAssign && match(TokenType::EQUAL)) {
                expression();
                if(arg <= 255) {
                    emitByte(OpCode::OP_SET_LOCAL);
                    emitByte(static_cast<std::uint8_t>(arg));
                } else {
                    emitByte(OpCode::OP_SET_LOCAL_LONG);
                    emitByte(static_cast<std::uint8_t>(arg & 0xFF));
                    emitByte(static_cast<std::uint8_t>((arg >> 8) & 0xFF));
                    emitByte(static_cast<std::uint8_t>((arg >> 16) & 0xFF));
                }
            } else {
                if(arg <= 255) {
                    emitByte(OpCode::OP_GET_LOCAL);
                    emitByte(static_cast<std::uint8_t>(arg));
                } else {
                    emitByte(OpCode::OP_GET_LOCAL_LONG);
                    emitByte(static_cast<std::uint8_t>(arg & 0xFF));
                    emitByte(static_cast<std::uint8_t>((arg >> 8) & 0xFF));
                    emitByte(static_cast<std::uint8_t>((arg >> 16) & 0xFF));
                }
            }
        }

    }
    void Compiler::and_(bool canAssign) {
        static_cast<void>(canAssign);
        std::size_t endJump{emitJump(OpCode::OP_JUMP_IF_FALSE)};

        emitByte(OpCode::OP_POP);
        parsePrecedence(Precedence::PREC_AND);
        patchJump(endJump);
    }

    void Compiler::or_(bool canAssign) {
        static_cast<void>(canAssign);
        std::size_t elseJump{emitJump(OpCode::OP_JUMP_IF_FALSE)};
        std::size_t endJump{emitJump(OpCode::OP_JUMP)};
        
        patchJump(elseJump);
        emitJump(OpCode::OP_POP);
        parsePrecedence(Precedence::PREC_OR);
        patchJump(endJump);
    }

    void Compiler::beginScope()
    {
        scopeDepth_++;
    }

    void Compiler::endScope() {
        scopeDepth_--;
        while(localCount_ > 0 && locals_[localCount_ - 1].depth > scopeDepth_) {
            emitByte(OpCode::OP_POP);
            localCount_--;
        }
    }
    
    void Compiler::block() {
        while(parser_.currentToken().type_ != TokenType::RIGHT_BRACE &&
              parser_.currentToken().type_ != TokenType::EOF_) {
                declaration();
              }
        parser_.consume(TokenType::RIGHT_BRACE, "expected '}' after block");
    }

    void Compiler::declareVariable() {
        std::string_view name{parser_.previousToken().lexeme_};

        for(int i{static_cast<int>(localCount_ - 1)}; i >= 0; i--) {
            std::size_t idx{static_cast<std::size_t>(i)};
            if(locals_[idx].depth != UNINITIALIZED && locals_[idx].depth < scopeDepth_) {
                break;
            }

            if(locals_[idx].name == name) {
                parser_.error("already a variable with this name in this scope");
            }
        }

        addLocal(name);
    }
    
    void Compiler::addLocal(std::string_view name) {
        if(localCount_ >= LOCAL_STACK_SIZE) {
            parser_.error("too many local variables");
        }

        locals_[localCount_] = Local{name, UNINITIALIZED};
        localCount_++;
    }
    
    void Compiler::markInitialized() {
        locals_[localCount_ - 1].depth = scopeDepth_;
    }
    
    int Compiler::resolveLocal(const Token &name) {
        for(int i{static_cast<int>(localCount_ - 1)}; i >= 0; i--) {
            if(locals_[static_cast<std::size_t>(i)].name == name.lexeme_) {
                if(locals_[static_cast<std::size_t>(i)].depth == UNINITIALIZED) {
                    parser_.error("can't read local variable in its own initializer");
                }

                return i;
            }
        }

        return -1;
    }
}