#include "vm.hpp"

namespace pegasus {
    InterpretResult VM::run() {
        try {
            while(true) {
                if constexpr(DEBUG_TRACE_EXECUTION) {
                    printf("%-10s", "");
                    for(Value* slot {stack_.data()}; slot < stackTop_; slot++) {
                        printf("[ ");
                        printValue(*slot);
                        printf(" ]");
                    }
                    printf("\n");
                    disassembleInstruction(chunk_, static_cast<std::size_t>(ip_ - chunk_->getCode()));
                }

                OpCode instruction{static_cast<OpCode>(*ip_++)};
                switch(instruction) {
                    case OpCode::OP_CONSTANT: {
                        const std::uint8_t constantIndex{*ip_++};
                        Value constant{chunk_->getConstant(constantIndex)};
                        push(constant);
                        break;
                    }
                    case OpCode::OP_CONSTANT_LONG: {
                        const std::size_t constantIndex{readConstantIndexLong()};
                        Value constant{chunk_->getConstant(constantIndex)};
                        push(constant);
                        break;
                    }

                    case OpCode::OP_DEFINE_GLOBAL: {
                        const std::size_t constantIndex{*ip_++};
                        Value identifier{chunk_->getConstant(constantIndex)};
                        std::string_view variableName{extractVariableName(identifier)};
                        std::string_view internedName{pool_.intern(variableName)};
                        globalVariables_[internedName] = peek(0);
                        pop();
                        break;
                    }

                    case OpCode::OP_DEFINE_GLOBAL_LONG: {
                        const std::size_t constantIndex{readConstantIndexLong()};
                        Value identifier{chunk_->getConstant(constantIndex)};
                        std::string_view variableName{extractVariableName(identifier)};
                        std::string_view internedName{pool_.intern(variableName)};
                        globalVariables_[internedName] = peek(0);
                        pop();
                        break;
                    }

                    case OpCode::OP_GET_GLOBAL: {
                        const std::size_t constantIndex{static_cast<std::size_t>(*ip_++)};
                        Value identifier{chunk_->getConstant(constantIndex)};
                        std::string_view variableName{extractVariableName(identifier)};
                        std::string_view internedName{pool_.intern(variableName)};

                        auto it{globalVariables_.find(internedName)};
                        if(it == globalVariables_.end()) {
                            throw std::runtime_error("undefined variable '" + std::string(internedName) + "'");
                        }

                        push(it->second);
                        break;
                    }

                    case OpCode::OP_GET_GLOBAL_LONG: {
                        const std::size_t constantIndex{readConstantIndexLong()};
                        Value identifier{chunk_->getConstant(constantIndex)};
                        std::string_view variableName{extractVariableName(identifier)};
                        std::string_view internedName{pool_.intern(variableName)};

                        auto it{globalVariables_.find(internedName)};
                        if(it == globalVariables_.end()) {
                            throw std::runtime_error("undefined variable '" + std::string(internedName) + "'");
                        }

                        push(it->second);
                        break;
                    }
                    case OpCode::OP_SET_GLOBAL: {
                        std::size_t constantIndex{static_cast<std::size_t>(*ip_++)};
                        Value identifier{chunk_->getConstant(constantIndex)};
                        std::string_view variableName{extractVariableName(identifier)};
                        std::string_view internedName{pool_.intern(variableName)};

                        auto it{globalVariables_.find(internedName)};
                        if(it == globalVariables_.end()) {
                            throw std::runtime_error("undefined variable '" + std::string(internedName) + "'");
                        }

                        it->second = peek(0);
                        break;
                    }

                    case OpCode::OP_SET_GLOBAL_LONG: {
                        const std::size_t constantIndex{readConstantIndexLong()};
                        Value identifier{chunk_->getConstant(constantIndex)};
                        std::string_view variableName{extractVariableName(identifier)};
                        std::string_view internedName{pool_.intern(variableName)};

                        auto it{globalVariables_.find(internedName)};
                        if(it == globalVariables_.end()) {
                            throw std::runtime_error("undefined variable '" + std::string(internedName) + "'");
                        }

                        it->second = peek(0);
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
                    case OpCode::OP_RETURN:
                        return InterpretResult::OK;
                    default:
                        return InterpretResult::RUNTIME_ERROR;
                }
            }
        } catch (const std::runtime_error& e) {
            printf("runtime error: %s\n", e.what());
            return InterpretResult::RUNTIME_ERROR;
        }
    }
    
    InterpretResult VM::interpret(std::string_view source) {
        Chunk chunk;
        if(!compile(source, chunk)) return InterpretResult::COMPILE_ERROR;
        chunk_ = &chunk;
        ip_ = chunk_->getCode();
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
        
        std::visit([&op, this](auto&& aVal, auto&& bVal) {
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
                throw std::runtime_error("operands must be of same type for binary operations");
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

    std::size_t VM::readConstantIndexLong() {
        const std::size_t lowByte{static_cast<std::size_t>(*ip_++)};
        const std::size_t midByte{static_cast<std::size_t>(*ip_++)};
        const std::size_t highByte{static_cast<std::size_t>(*ip_++)};
        return (highByte << 16) | (midByte << 8) | lowByte;
    }
}