#include "vm.hpp"

namespace pegasus {
    InterpretResult VM::run() {
        try {
            CallFrame* frame{&frames_[frameCount_ - 1]};
            while(true) {
                if constexpr(DEBUG_TRACE_EXECUTION) {
                    printf("%-10s", "");
                    for(Value* slot {stack_.data()}; slot < stackTop_; slot++) {
                        printf("[ ");
                        printValue(*slot);
                        printf(" ]");
                    }
                    printf("\n");
                    disassembleInstruction(&currentFunction(*frame).chunk_, static_cast<std::size_t>(frame->ip_ - currentFunction(*frame).chunk_.getCode()));
                }

                OpCode instruction{static_cast<OpCode>(*frame->ip_++)};
                switch(instruction) {
                    case OpCode::OP_CONSTANT: {
                        const std::uint8_t constantIndex{*frame->ip_++};
                        Value constant{currentFunction(*frame).chunk_.getConstant(constantIndex)};
                        push(constant);
                        break;
                    }
                    case OpCode::OP_CONSTANT_LONG: {
                        const std::size_t constantIndex{readConstantIndexLong(frame->ip_)};
                        Value constant{currentFunction(*frame).chunk_.getConstant(constantIndex)};
                        push(constant);
                        break;
                    }

                    case OpCode::OP_DEFINE_GLOBAL: {
                        const std::size_t constantIndex{*frame->ip_++};
                        Value identifier{currentFunction(*frame).chunk_.getConstant(constantIndex)};
                        std::string_view variableName{extractVariableName(identifier)};
                        std::string_view internedName{strPool_.intern(variableName)};
                        globalVariables_[internedName] = peek(0);
                        pop();
                        break;
                    }

                    case OpCode::OP_DEFINE_GLOBAL_LONG: {
                        const std::size_t constantIndex{readConstantIndexLong(frame->ip_)};
                        Value identifier{currentFunction(*frame).chunk_.getConstant(constantIndex)};
                        std::string_view variableName{extractVariableName(identifier)};
                        std::string_view internedName{strPool_.intern(variableName)};
                        globalVariables_[internedName] = peek(0);
                        pop();
                        break;
                    }

                    case OpCode::OP_GET_GLOBAL: {
                        const std::size_t constantIndex{static_cast<std::size_t>(*frame->ip_++)};
                        Value identifier{currentFunction(*frame).chunk_.getConstant(constantIndex)};
                        std::string_view variableName{extractVariableName(identifier)};
                        std::string_view internedName{strPool_.intern(variableName)};

                        auto it{globalVariables_.find(internedName)};
                        if(it == globalVariables_.end()) {
                            throw std::runtime_error("undefined variable '" + std::string(internedName) + "'");
                        }

                        push(it->second);
                        break;
                    }

                    case OpCode::OP_GET_GLOBAL_LONG: {
                        const std::size_t constantIndex{readConstantIndexLong(frame->ip_)};
                        Value identifier{currentFunction(*frame).chunk_.getConstant(constantIndex)};
                        std::string_view variableName{extractVariableName(identifier)};
                        std::string_view internedName{strPool_.intern(variableName)};

                        auto it{globalVariables_.find(internedName)};
                        if(it == globalVariables_.end()) {
                            throw std::runtime_error("undefined variable '" + std::string(internedName) + "'");
                        }

                        push(it->second);
                        break;
                    }
                    case OpCode::OP_SET_GLOBAL: {
                        const std::size_t constantIndex{static_cast<std::size_t>(*frame->ip_++)};
                        Value identifier{currentFunction(*frame).chunk_.getConstant(constantIndex)};
                        std::string_view variableName{extractVariableName(identifier)};
                        std::string_view internedName{strPool_.intern(variableName)};

                        auto it{globalVariables_.find(internedName)};
                        if(it == globalVariables_.end()) {
                            throw std::runtime_error("undefined variable '" + std::string(internedName) + "'");
                        }

                        it->second = peek(0);
                        break;
                    }

                    case OpCode::OP_SET_GLOBAL_LONG: {
                        const std::size_t constantIndex{readConstantIndexLong(frame->ip_)};
                        Value identifier{currentFunction(*frame).chunk_.getConstant(constantIndex)};
                        std::string_view variableName{extractVariableName(identifier)};
                        std::string_view internedName{strPool_.intern(variableName)};

                        auto it{globalVariables_.find(internedName)};
                        if(it == globalVariables_.end()) {
                            throw std::runtime_error("undefined variable '" + std::string(internedName) + "'");
                        }

                        it->second = peek(0);
                        break;
                    }

                    case OpCode::OP_GET_LOCAL: {
                        const std::uint8_t slot{*frame->ip_++};
                        push(frame->slots_[slot]);
                        break;
                    }

                    case OpCode::OP_GET_LOCAL_LONG: {
                        const std::size_t slot{readConstantIndexLong(frame->ip_)};
                        push(frame->slots_[slot]);
                        break;
                    }

                    case OpCode::OP_SET_LOCAL: {
                        const std::uint8_t slot{*frame->ip_++};
                        frame->slots_[slot] = peek(0);
                        break;
                    }

                    case OpCode::OP_SET_LOCAL_LONG: {
                        const std::size_t slot{readConstantIndexLong(frame->ip_)};
                        frame->slots_[slot] = peek(0);
                        break;
                    }

                    case OpCode::OP_JUMP: {
                        const std::uint8_t lsb{*frame->ip_++};
                        const std::uint8_t msb{*frame->ip_++};
                        const std::uint16_t offset{static_cast<std::uint16_t>((msb << 8) | lsb)};
                        frame->ip_ += offset;
                        break;
                    }
                    
                    case OpCode::OP_JUMP_IF_FALSE: {
                        const std::uint8_t lsb{*frame->ip_++};
                        const std::uint8_t msb{*frame->ip_++};
                        const std::uint16_t offset{static_cast<std::uint16_t>((msb << 8) | lsb)};
                        
                        if(isFalsey(peek(0))) frame->ip_ += offset;
                        break;
                    }
                    
                    case OpCode::OP_LOOP: {
                        const std::uint8_t lsb{*frame->ip_++};
                        const std::uint8_t msb{*frame->ip_++};
                        const std::uint16_t offset{static_cast<std::uint16_t>((msb << 8) | lsb)};
                        frame->ip_ -= offset;
                        break;
                    }

                    case OpCode::OP_CALL: {
                        std::uint8_t argCount{*frame->ip_++};
                        Value callee{peek(argCount)};

                        if(!callValue(callee, argCount)) {
                            return InterpretResult::RUNTIME_ERROR;
                        }

                        frame = &frames_[frameCount_ - 1];
                        break;
                    }

                    case OpCode::OP_ADD:        {binaryOp(BinaryOp::ADD);break;}
                    case OpCode::OP_SUBTRACT:   {binaryOp(BinaryOp::SUBTRACT);break;}
                    case OpCode::OP_MULTIPLY:   {binaryOp(BinaryOp::MULTIPLY);break;}
                    case OpCode::OP_DIVIDE:     {binaryOp(BinaryOp::DIVIDE);break;}
                    case OpCode::OP_NEGATE:     {push(negateValue(pop()));break;}
                    case OpCode::OP_TRUE:       {push(Value{true});break;}
                    case OpCode::OP_FALSE:      {push(Value{false});break;}
                    case OpCode::OP_NIL:        {push(Value{std::monostate{}});break;}
                    case OpCode::OP_NOT:        {push(notValue(pop()));break;}
                    case OpCode::OP_EQUAL:      {binaryOp(BinaryOp::EQUAL);break;}
                    case OpCode::OP_GREATER:    {binaryOp(BinaryOp::GREATER);break;}
                    case OpCode::OP_LESS:       {binaryOp(BinaryOp::LESS);break;}
                    case OpCode::OP_PRINT:      {printValue(pop());printf("\n");break;}
                    case OpCode::OP_POP:        {pop();break;}
                    case OpCode::OP_RETURN: {
                        Value result{pop()};
                        frameCount_--;
                        if(frameCount_ == 0) {
                            pop();
                            return InterpretResult::OK;
                        }
                        stackTop_ = frame->slots_;
                        push(result);
                        frame = &frames_[frameCount_ - 1];
                        break;
                    }
                    default:
                        return InterpretResult::RUNTIME_ERROR;
                }
            }
        } catch (const std::runtime_error& e) {
            runtimeError(e.what());
            return InterpretResult::RUNTIME_ERROR;
        }
    }
    
    InterpretResult VM::interpret(std::string_view source) {
        auto function{compile(source, funcPool_)};
        if(!function) return InterpretResult::COMPILE_ERROR;

        FunctionIndex funcIndex{funcPool_.addFunction(std::move(*function))};
        CallFrame& frame{frames_[frameCount_++]};
        frame.funcIndex_ = funcIndex;
        frame.ip_ = funcPool_.getFunction(funcIndex).chunk_.getCode();
        frame.slots_ = stack_.data();

        return run();
    }

    void VM::push(Value value)
    {
        if(stackTop_ >= stack_.data() + STACK_SIZE) {
            throw std::runtime_error("stack overflow");
        }
        *stackTop_ = value;
        stackTop_++;
    }

    Value VM::pop() {
        if(stackTop_ == stack_.data()) {
            throw std::runtime_error("stack underflow");
        }
        stackTop_--;
        return *stackTop_;
    }

    Value VM::peek(int distance) {
        if(stackTop_ - distance - 1 < stack_.data()) {
            throw std::runtime_error("invalid peek of stack");
        }
        return stackTop_[-distance - 1];
    }

    void VM::binaryOp(BinaryOp op) {
        Value b{pop()};
        Value a{pop()};
        
        std::visit([&a, &b, &op, this](auto&& aVal, auto&& bVal) {
            using A = std::decay_t<decltype(aVal)>;
            using B = std::decay_t<decltype(bVal)>;

            if constexpr(std::is_arithmetic_v<A> && std::is_arithmetic_v<B>) {
                switch(op) {
                    case BinaryOp::ADD:         {push(Value{aVal + bVal});break;}
                    case BinaryOp::SUBTRACT:    {push(Value{aVal - bVal});break;}
                    case BinaryOp::MULTIPLY:    {push(Value{aVal * bVal});break;}
                    case BinaryOp::DIVIDE:      {push(Value{aVal / bVal});break;}
                    case BinaryOp::EQUAL:       {push(Value{aVal == bVal});break;}
                    case BinaryOp::GREATER:     {push(Value{aVal > bVal});break;}
                    case BinaryOp::LESS:        {push(Value{aVal < bVal});break;}
                    default:
                        throw std::runtime_error("unknown binary operator");
                }
            } else if constexpr(std::is_same_v<A, std::string_view> && std::is_same_v<B, std::string_view>) {
                switch(op) {
                    case BinaryOp::ADD:         {push(Value{std::string{aVal} + std::string{bVal}});break;}
                    case BinaryOp::EQUAL:       {push(Value{aVal == bVal});break;}
                    case BinaryOp::GREATER:     {push(Value{aVal > bVal});break;}
                    case BinaryOp::LESS:        {push(Value{aVal < bVal});break;}
                    default:
                        throw std::runtime_error("invalid operator for strings");
                }

            } else if constexpr(std::is_same_v<A, std::string> && std::is_same_v<B, std::string>) {
                switch(op) {
                    case BinaryOp::ADD:         {push(Value{std::string{aVal} + std::string{bVal}});break;}
                    case BinaryOp::EQUAL:       {push(Value{aVal == bVal});break;}
                    case BinaryOp::GREATER:     {push(Value{aVal > bVal});break;}
                    case BinaryOp::LESS:        {push(Value{aVal < bVal});break;}
                    default:
                        throw std::runtime_error("invalid operator for strings");
                }
            } else {
                    throw std::runtime_error("operands must be of same type for binary operations: " + 
                    std::to_string(a.index()) + " and " + std::to_string(b.index()));
            }

        }, a, b);
    }

    std::string_view VM::extractVariableName(const Value &identifier) {
        return std::visit([](auto&& v) -> std::string_view {
            using T = std::decay_t<decltype(v)>;
            if constexpr(std::is_same_v<T, std::string_view> || std::is_same_v<T, std::string>) {
                return v;
            } else {
                throw std::runtime_error("identifier must be a string");
            }
        }, identifier);
    }

    std::size_t VM::readConstantIndexLong(const std::uint8_t* frameIp) {
        const std::size_t lowByte{static_cast<std::size_t>(*frameIp++)};
        const std::size_t midByte{static_cast<std::size_t>(*frameIp++)};
        const std::size_t highByte{static_cast<std::size_t>(*frameIp++)};
        return (highByte << 16) | (midByte << 8) | lowByte;
    }

    bool VM::callValue(const Value &callee, std::uint8_t argCount) {
        if(std::holds_alternative<NativeFunction>(callee)) {
            const NativeFunction& nativeFunc{std::get<NativeFunction>(callee)};
            Value result{nativeFunc.function_(argCount, stackTop_ - argCount)};
            stackTop_ -= argCount + 1;
            push(result);
            return true;
        }


        if(!std::holds_alternative<FunctionIndex>(callee)) {
            throw std::runtime_error("can only call functions");
        }

        const FunctionIndex funcIndex{std::get<FunctionIndex>(callee)};
        const ObjFunction& function{funcPool_.getFunction(funcIndex)};

        if(function.arity_ != argCount) {
            throw std::runtime_error("expected " + std::to_string(function.arity_) + " arguments but got " + std::to_string(argCount));
        }

        if(frameCount_ >= FRAME_MAX) {
            throw std::runtime_error("stack overflow");
        }

        CallFrame& frame{frames_[frameCount_++]};
        frame.funcIndex_ = funcIndex;
        frame.ip_ = function.chunk_.getCode();
        frame.slots_ = stackTop_ - argCount - 1;
        return true;
    }

    const ObjFunction& VM::currentFunction(const CallFrame &frame) {
        return funcPool_.getFunction(frame.funcIndex_);
    }
    
    void VM::runtimeError(std::string_view errorMessage) {
        fprintf(stderr, "%s\n", errorMessage.data());

        for(std::size_t i{frameCount_}; i > 0; i--) {
            const CallFrame& frame{frames_[i - 1]};
            const ObjFunction& objFunc{funcPool_.getFunction(frame.funcIndex_)};
            std::size_t offset{static_cast<std::size_t>(frame.ip_ - objFunc.chunk_.getCode())};
            fprintf(stderr, "[line %d] in ", objFunc.chunk_.getLine(offset));

            if(objFunc.name_.empty()) {
                fprintf(stderr, "script\n");
            } else {
                fprintf(stderr, "%s()\n", objFunc.name_.c_str());
            }
        }
    }

    static Value clockNative(std::size_t argCount, Value *args) {
        static_cast<void>(argCount);
        static_cast<void>(args);
        auto now{std::chrono::high_resolution_clock::now()};
        auto duration{now.time_since_epoch()};
        double seconds{std::chrono::duration<double>(duration).count()};
        return Value{seconds};
    }

    void VM::defineNatives() {
        globalVariables_[strPool_.intern("clock")] = Value{NativeFunction{clockNative}};
    }
}