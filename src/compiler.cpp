#include "compiler.hpp"

namespace pegasus {
    

    const std::array<Compiler::ParseRule, Compiler::TOKEN_COUNT> Compiler::rules {{

        /* LEFT_PAREN */    {&Compiler::grouping, &Compiler::call, Compiler::Precedence::PREC_CALL},
        /* RIGHT_PAREN */   {nullptr, nullptr, Compiler::Precedence::PREC_NONE},
        /* LEFT_BRACE */    {nullptr, nullptr, Compiler::Precedence::PREC_NONE},
        /* RIGHT_BRACE */   {nullptr, nullptr, Compiler::Precedence::PREC_NONE},
        /* COMMA */         {nullptr, nullptr, Compiler::Precedence::PREC_NONE},
        /* DOT */           {nullptr, &Compiler::dot, Compiler::Precedence::PREC_CALL},
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
        /* FN */            {nullptr, nullptr, Compiler::Precedence::PREC_NONE},
        /* CLASS */         {nullptr, nullptr, Compiler::Precedence::PREC_NONE},
        /* PRINT */         {nullptr, nullptr, Compiler::Precedence::PREC_NONE},
        /* RETURN */        {nullptr, nullptr, Compiler::Precedence::PREC_NONE},
        /* VAR */           {nullptr, nullptr, Compiler::Precedence::PREC_NONE},
        /* THIS */          {&Compiler::this_, nullptr, Compiler::Precedence::PREC_NONE},
        /* NIL */           {&Compiler::literal, nullptr, Compiler::Precedence::PREC_NONE},
        /* SUPER */         {nullptr, nullptr, Compiler::Precedence::PREC_NONE},

        /* ERROR */         {nullptr, nullptr, Compiler::Precedence::PREC_NONE},
         /* EOF_ */         {nullptr, nullptr, Compiler::Precedence::PREC_NONE}
    }};

    std::optional<ObjFunction> Compiler::compile() {
        functionType_ = FunctionType::TYPE_SCRIPT;
        parser_.advance();
        while(!match(TokenType::EOF_)) declaration();
        endCompiler();
        return parser_.hadError() ? std::nullopt : std::optional{std::move(function_)};
    }

    int Compiler::addUpvalue(std::uint8_t index, bool isLocal) {
        std::size_t upvalueCount{function_.upvalueCount};

        for(std::size_t i{0}; i < upvalueCount; i++) {
            if(upvalues_[i].index == index && upvalues_[i].isLocal == isLocal) {
                return static_cast<int>(i);
            }
        }

        if(upvalueCount >= 256) {
            parser_.error("too many closure variables in function");
        }

        upvalues_[upvalueCount].isLocal = isLocal;
        upvalues_[upvalueCount].index = index;
        function_.upvalueCount++;
        return static_cast<int>(upvalueCount);
    }

    int Compiler::resolveUpvalue(Compiler* compiler, const Token& name) {
        if(compiler->enclosing_ == nullptr) return -1;

        // check if the variable is a local in the immediately enclosing function
        int local{compiler->enclosing_->resolveLocal(name)};
        if(local != -1) {                                        // local: index of where upvalue is inside parent functions local array
            compiler->enclosing_->locals_[static_cast<std::size_t>(local)].isCaptured = true;
            return compiler->addUpvalue(static_cast<std::uint8_t>(local), true);
        }

        // check if it's already an upvalue in the enclosing function
        int upvalue{resolveUpvalue(compiler->enclosing_, name)};
        if(upvalue != -1) {                                      // upvalue: index of where upvalue is inside parent
            return compiler->addUpvalue(static_cast<std::uint8_t>(upvalue), false);
        }

        return -1;
    }

    const Compiler::ParseRule &Compiler::getRule(TokenType type)
    {
        return rules[static_cast<std::size_t>(type)];
    }

    Chunk* Compiler::currentChunk() { return &function_.chunk; }

    void Compiler::endCompiler() { 
        emitReturn();
        if(DEBUG_PRINT_CODE && !parser_.hadError()) {
            disassembleChunk(currentChunk(), !function_.name.empty() ? function_.name : "<script>");
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
        if(functionType_ == FunctionType::TYPE_INITIALIZER) {
            emitByte(OpCode::OP_GET_LOCAL);
            emitByte(static_cast<std::uint8_t>(0)); // slot 0 is 'this'
        } else {
            emitByte(OpCode::OP_NIL);
        }
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
        if(match(TokenType::CLASS)) {
            classDeclaration();
        } else if(match(TokenType::FN)) {
            fnDeclaration();
        } else if(match(TokenType::VAR)) {
            varDeclaration();
        } else {
            statement();
        }

        if(parser_.isInPanicMode()) synchronize();
    }

    void Compiler::classDeclaration() {
        parser_.consume(TokenType::IDENTIFIER, "expect class name");
        Token className{parser_.previousToken()};
        std::size_t nameConstant{currentChunk()->addConstant(Value{std::string(className.lexeme_)})};

        if(scopeDepth_ > 0) {
            declareLocalVariable();
            initializeLocal();
        }

        ClassCompiler classCompiler;
        classCompiler.enclosing = currentClass_;
        currentClass_ = &classCompiler;

        emitByte(OpCode::OP_CLASS);
        emitByte(static_cast<std::uint8_t>(nameConstant));

        if(scopeDepth_ > 0) {
            // local already declared and initialized above
        } else {
            // global
            if(nameConstant <= 255) {
                emitByte(OpCode::OP_DEFINE_GLOBAL);
                emitByte(static_cast<std::uint8_t>(nameConstant));
            } else {
                emitByte(OpCode::OP_DEFINE_GLOBAL_LONG);
                emitByte(static_cast<std::uint8_t>(nameConstant & 0xFF));
                emitByte(static_cast<std::uint8_t>((nameConstant >> 8) & 0xFF));
                emitByte(static_cast<std::uint8_t>((nameConstant >> 16) & 0xFF));
            }
        }

        namedVariable(className, false);

        parser_.consume(TokenType::LEFT_BRACE, "expect '{' before class body");
        while(parser_.currentToken().type_ != TokenType::RIGHT_BRACE && parser_.currentToken().type_ != TokenType::EOF_) {
            method();
        }        
        parser_.consume(TokenType::RIGHT_BRACE, "expect '}' after class body");
    
        emitByte(OpCode::OP_POP); // pop the class

        currentClass_ = classCompiler.enclosing; // restore
    }

    void Compiler::fnDeclaration() {
        std::size_t globalIndex{parseVariable("expect function name")};
        initializeLocal();
        function(FunctionType::TYPE_FUNCTION);
        defineVariable(globalIndex);
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
            declareLocalVariable();
            return 0;
        }
        
        return currentChunk()->writeConstant(Value{parser_.previousToken().lexeme_}, parser_.previousLine());
    }

    void Compiler::defineVariable(const std::size_t global) {
        if(scopeDepth_ > 0) {
            initializeLocal();
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
        } else if(match(TokenType::RETURN)) {
            returnStatement();
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
                case TokenType::FN:
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

    void Compiler::returnStatement() {
        if(functionType_ == FunctionType::TYPE_SCRIPT) {
            parser_.error("can't return from top-level code");
        }

        if(match(TokenType::SEMICOLON)) {
            emitReturn();
        } else {
            if(functionType_ == FunctionType::TYPE_INITIALIZER) {
                parser_.error("can't return a value from an initializer");
            }
            expression();
            parser_.consume(TokenType::SEMICOLON, "expect ';' after return value");
            emitByte(OpCode::OP_RETURN);
        }
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

    void Compiler::dot(bool canAssign) {
        parser_.consume(TokenType::IDENTIFIER, "expect property name after '.'");
        std::size_t nameConstant{currentChunk()->addConstant(
            Value{std::string(parser_.previousToken().lexeme_)}
        )};

        if(canAssign && match(TokenType::EQUAL)) {
            expression();
            emitByte(OpCode::OP_SET_PROPERTY);
            emitByte(static_cast<std::uint8_t>(nameConstant));
        } else if(match(TokenType::LEFT_PAREN)) {
            // optimized invoke: obj.method(args) in one step
            std::uint8_t argCount{argumentList()};
            emitByte(OpCode::OP_INVOKE);
            emitByte(static_cast<std::uint8_t>(nameConstant));
            emitByte(argCount);
        } else {
            emitByte(OpCode::OP_GET_PROPERTY);
            emitByte(static_cast<std::uint8_t>(nameConstant));
        }
    }

    void Compiler::variable(bool canAssign) {
        namedVariable(parser_.previousToken(), canAssign);
    }

    void Compiler::namedVariable(const Token& name, bool canAssign) {
        static_cast<void>(canAssign);
        int arg{resolveLocal(name)};
        bool isLocal{arg != -1};

        if(isLocal) {
            // local variable
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

        } else if((arg = resolveUpvalue(this, name)) != -1) {
            // upvalue
            if(canAssign && match(TokenType::EQUAL)) {
                expression();
                emitByte(OpCode::OP_SET_UPVALUE);
                emitByte(static_cast<std::uint8_t>(arg));
            } else {
                emitByte(OpCode::OP_GET_UPVALUE);
                emitByte(static_cast<std::uint8_t>(arg));
            }
        } else {
            // global variable
            std::size_t globalArg{currentChunk()->addConstant(Value{name.lexeme_})};

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

    void Compiler::this_(bool canAssign) {
        static_cast<void>(canAssign);
        if(currentClass_ == nullptr) {
            parser_.error("can't use 'this' outside of a class");
            return;
        }
        variable(false); // 'this' is just a local variable lookup
    }

    void Compiler::call(bool canAssign) {
        static_cast<void>(canAssign);
        std::uint8_t argCount{argumentList()};
        emitByte(OpCode::OP_CALL);
        emitByte(argCount);
    }

    void Compiler::beginScope() {
        scopeDepth_++;
    }

    void Compiler::endScope() {
        scopeDepth_--;
        while(localCount_ > 0 && locals_[localCount_ - 1].depth > scopeDepth_) {
            if(locals_[localCount_ - 1].isCaptured) {
                emitByte(OpCode::OP_CLOSE_UPVALUE);
            } else {
                emitByte(OpCode::OP_POP);
            }
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

    void Compiler::function(FunctionType funcType) {
        Compiler compiler{parser_, funcPool_};
        compiler.enclosing_ = this; // link to enclosing compiler
        compiler.functionType_ = funcType;
        compiler.function_.name = std::string(compiler.parser_.previousToken().lexeme_);

        compiler.beginScope();
        if(funcType == FunctionType::TYPE_METHOD || funcType == FunctionType::TYPE_INITIALIZER) {
            compiler.addLocal("this");
        } else {
            compiler.addLocal("");
        }
        compiler.initializeLocal();
        compiler.parser_.consume(TokenType::LEFT_PAREN, "expect '(' after function name");
        if(compiler.parser_.currentToken().type_ != TokenType::RIGHT_PAREN) {
            do {
                if(compiler.function_.arity == 255) {
                    compiler.parser_.errorAtCurrent("can't have more than 255 parameters");
                }
                compiler.function_.arity++;
                std::size_t constant{compiler.parseVariable("expect parameter name")};
                compiler.defineVariable(constant);
            } while(compiler.match(TokenType::COMMA));
        }
        compiler.parser_.consume(TokenType::RIGHT_PAREN, "expect ')' after parameters");
        compiler.parser_.consume(TokenType::LEFT_BRACE, "expect '{' before function body");
        compiler.block();
        compiler.endCompiler();

        std::uint8_t upvalueCount{compiler.function_.upvalueCount};
        FunctionIndex funcIndex{funcPool_.addFunction(std::move(compiler.function_))};
        std::size_t constant{currentChunk()->addConstant(Value{funcIndex})};
        emitByte(OpCode::OP_CLOSURE);
        emitByte(static_cast<std::uint8_t>(constant));

        for(std::size_t i{0}; i < upvalueCount; i++) {
            emitByte(compiler.upvalues_[i].isLocal ? 1 : 0);
            emitByte(compiler.upvalues_[i].index);
        }
    }

    void Compiler::method() {
        parser_.consume(TokenType::IDENTIFIER, "expect method name");
        std::string_view methodName{parser_.previousToken().lexeme_};
        std::size_t nameConstant{currentChunk()->addConstant(
            Value{std::string(parser_.previousToken().lexeme_)}
        )};

        FunctionType type{FunctionType::TYPE_METHOD};
        if(methodName == "init") {
            type = FunctionType::TYPE_INITIALIZER;
        }

        function(type);

        emitByte(OpCode::OP_METHOD);
        emitByte(static_cast<std::uint8_t>(nameConstant));
    }

    void Compiler::declareLocalVariable() {
        std::string_view name{parser_.previousToken().lexeme_};

        for(int i{static_cast<int>(localCount_ - 1)}; i >= 0; i--) {
            std::size_t index{static_cast<std::size_t>(i)};
            if(locals_[index].depth != UNINITIALIZED && locals_[index].depth < scopeDepth_) {
                break;
            }

            if(locals_[index].name == name) {
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
    
    void Compiler::initializeLocal() {
        if(scopeDepth_ == 0) return;
        locals_[localCount_ - 1].depth = scopeDepth_;
    }
    
    int Compiler::resolveLocal(const Token &name) {
        for(std::size_t i{localCount_}; i > 0; i--) {
            std::size_t index{i - 1};
            if(locals_[index].name == name.lexeme_) {
                if(locals_[index].depth == UNINITIALIZED) {
                    parser_.error("can't read local variable in its own initializer");
                }

                return static_cast<int>(index);
            }
        }

        return -1;
    }
    
    std::uint8_t Compiler::argumentList() {
        std::uint8_t argCount{0};

        if(parser_.currentToken().type_ != TokenType::RIGHT_PAREN) {
            do {
                expression();
                if(argCount == 255) {
                    parser_.error("can't have more than 255 arguments");
                }
                argCount++;
            } while(match(TokenType::COMMA));
        }

        parser_.consume(TokenType::RIGHT_PAREN, "expect ')' after arguments");
        return argCount;
    }
}