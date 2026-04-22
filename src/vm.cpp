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
                    disassembleInstruction(&currentFunction(*frame).chunk, static_cast<std::size_t>(frame->ip - currentFunction(*frame).chunk.getCode()));
                }

                OpCode instruction{static_cast<OpCode>(*frame->ip++)};
                switch(instruction) {
                    case OpCode::OP_CONSTANT: {
                        const std::uint8_t constantIndex{*frame->ip++};
                        Value constant{currentFunction(*frame).chunk.getConstant(constantIndex)};
                        push(constant);
                        break;
                    }
                    case OpCode::OP_CONSTANT_LONG: {
                        const std::size_t constantIndex{readConstantIndexLong(frame->ip)};
                        Value constant{currentFunction(*frame).chunk.getConstant(constantIndex)};
                        push(constant);
                        break;
                    }

                    case OpCode::OP_DEFINE_GLOBAL: {
                        const std::size_t constantIndex{*frame->ip++};
                        Value identifier{currentFunction(*frame).chunk.getConstant(constantIndex)};
                        std::string_view variableName{extractVariableName(identifier)};
                        std::string_view internedName{strPool_.intern(variableName)};
                        globalVariables_[internedName] = peek(0);
                        pop();
                        break;
                    }

                    case OpCode::OP_DEFINE_GLOBAL_LONG: {
                        const std::size_t constantIndex{readConstantIndexLong(frame->ip)};
                        Value identifier{currentFunction(*frame).chunk.getConstant(constantIndex)};
                        std::string_view variableName{extractVariableName(identifier)};
                        std::string_view internedName{strPool_.intern(variableName)};
                        globalVariables_[internedName] = peek(0);
                        pop();
                        break;
                    }

                    case OpCode::OP_GET_GLOBAL: {
                        const std::size_t constantIndex{static_cast<std::size_t>(*frame->ip++)};
                        Value identifier{currentFunction(*frame).chunk.getConstant(constantIndex)};
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
                        const std::size_t constantIndex{readConstantIndexLong(frame->ip)};
                        Value identifier{currentFunction(*frame).chunk.getConstant(constantIndex)};
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
                        const std::size_t constantIndex{static_cast<std::size_t>(*frame->ip++)};
                        Value identifier{currentFunction(*frame).chunk.getConstant(constantIndex)};
                        std::string_view variableName{extractVariableName(identifier)};
                        std::string_view internedName{strPool_.intern(variableName)};

                        auto it{globalVariables_.find(internedName)};
                        if(it == globalVariables_.end()) {
                            throw std::runtime_error("undefined variable '" + std::string(internedName) + "'");
                        }

                        if(immutableGlobals_.count(std::string(internedName))) {
                            throw std::runtime_error("cannot assign to immutable variable '" + std::string(internedName) + "'");
                        }

                        it->second = peek(0);
                        break;
                    }

                    case OpCode::OP_SET_GLOBAL_LONG: {
                        const std::size_t constantIndex{readConstantIndexLong(frame->ip)};
                        Value identifier{currentFunction(*frame).chunk.getConstant(constantIndex)};
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
                        const std::uint8_t slot{*frame->ip++};
                        push(frame->slots[slot]);
                        break;
                    }

                    case OpCode::OP_GET_LOCAL_LONG: {
                        const std::size_t slot{readConstantIndexLong(frame->ip)};
                        push(frame->slots[slot]);
                        break;
                    }

                    case OpCode::OP_SET_LOCAL: {
                        const std::uint8_t slot{*frame->ip++};
                        frame->slots[slot] = peek(0);
                        break;
                    }

                    case OpCode::OP_SET_LOCAL_LONG: {
                        const std::size_t slot{readConstantIndexLong(frame->ip)};
                        frame->slots[slot] = peek(0);
                        break;
                    }

                    case OpCode::OP_JUMP: {
                        const std::uint8_t lsb{*frame->ip++};
                        const std::uint8_t msb{*frame->ip++};
                        const std::uint16_t offset{static_cast<std::uint16_t>((msb << 8) | lsb)};
                        frame->ip += offset;
                        break;
                    }
                    
                    case OpCode::OP_JUMP_IF_FALSE: {
                        const std::uint8_t lsb{*frame->ip++};
                        const std::uint8_t msb{*frame->ip++};
                        const std::uint16_t offset{static_cast<std::uint16_t>((msb << 8) | lsb)};
                        
                        if(isFalsey(peek(0))) frame->ip += offset;
                        break;
                    }
                    
                    case OpCode::OP_LOOP: {
                        const std::uint8_t lsb{*frame->ip++};
                        const std::uint8_t msb{*frame->ip++};
                        const std::uint16_t offset{static_cast<std::uint16_t>((msb << 8) | lsb)};
                        frame->ip -= offset;
                        break;
                    }

                    case OpCode::OP_CALL: {
                        std::uint8_t argCount{*frame->ip++};
                        Value callee{peek(argCount)};

                        if(!callValue(callee, argCount)) {
                            return InterpretResult::RUNTIME_ERROR;
                        }

                        frame = &frames_[frameCount_ - 1];
                        break;
                    }

                    case OpCode::OP_CLOSURE: {
                        std::uint8_t constantIndex{*frame->ip++};
                        Value constant{currentFunction(*frame).chunk.getConstant(constantIndex)};
                        FunctionIndex funcIndex{std::get<FunctionIndex>(constant)};
                        const ObjFunction& function{funcPool_.getFunction(funcIndex)};
                    
                        ObjClosure closure;
                        closure.funcIndex = funcIndex;
                        closure.upvalues.resize(function.upvalueCount);

                        for(std::size_t i{0}; i < function.upvalueCount; i++) {
                            std::uint8_t isLocal{*frame->ip++};
                            std::uint8_t index{*frame->ip++};
                            if(isLocal) {                      
                                closure.upvalues[i] = captureUpvalue(frame->slots + index);
                            } else {                           
                                ObjClosure& enclosing{closurePool_.getClosure(frame->closureIndex)};
                                closure.upvalues[i] = enclosing.upvalues[index];
                            }                                                                                                        
                        }

                        ClosureIndex closureIndex{closurePool_.addClosure(std::move(closure))};
                        push(Value{closureIndex});
                        break;
                    }

                    case OpCode::OP_GET_UPVALUE: {
                        std::uint8_t slot{*frame->ip++};
                        ObjClosure& closure{closurePool_.getClosure(frame->closureIndex)};
                        ObjUpValue& uv{upvaluePool_.getUpvalue(closure.upvalues[slot])};
                        push(*uv.location);
                        break;
                    }

                    case OpCode::OP_SET_UPVALUE: {
                        std::uint8_t slot{*frame->ip++};
                        ObjClosure& closure{closurePool_.getClosure(frame->closureIndex)};
                        ObjUpValue& uv{upvaluePool_.getUpvalue(closure.upvalues[slot])};
                        *uv.location = peek(0);
                        break;
                    }

                    case OpCode::OP_CLOSE_UPVALUE: {
                        closeUpvalues(stackTop_ - 1);
                        pop();
                        break;   
                    }

                    case OpCode::OP_CLASS: {
                        std::uint8_t nameIndex{*frame->ip++};
                        Value nameVal{currentFunction(*frame).chunk.getConstant(nameIndex)};
                        std::string name{std::get<std::string>(nameVal)};

                        ObjClass cls;
                        cls.name = std::move(name);
                        ClassIndex clsIndex{classPool_.addClass(std::move(cls))};
                        push(Value{clsIndex});
                        break;
                    }

                    case OpCode::OP_GET_PROPERTY: {
                        if(!std::holds_alternative<InstanceIndex>(peek(0))) {
                            throw std::runtime_error("only instances have properties");
                        }

                        InstanceIndex instIndex{std::get<InstanceIndex>(peek(0))};
                        ObjInstance& instance{instancePool_.getInstance(instIndex)};

                        std::uint8_t nameIndex{*frame->ip++};
                        Value nameVal{currentFunction(*frame).chunk.getConstant(nameIndex)};
                        std::string name{std::visit([](auto&& v) -> std::string {
                            using T = std::decay_t<decltype(v)>;
                            if constexpr(std::is_same_v<T, std::string>) return v;
                            else if constexpr(std::is_same_v<T, std::string_view>) return std::string(v);
                            else throw std::runtime_error("property name must be a string");
                        }, nameVal)};

                        auto it{instance.fields.find(name)};
                        if(it != instance.fields.end()) {
                            pop(); // pop the instance
                            push(it->second); // push the field value
                            break;
                        }

                        ObjClass& cls{classPool_.getClass(instance.classIndex)};
                        auto methodIt{cls.methods.find(name)};
                        if(methodIt != cls.methods.end()) {
                            ObjBoundMethod bm;
                            bm.receiver = instIndex;
                            bm.method = methodIt->second;
                            BoundMethodIndex bmIndex{boundMethodPool_.addBoundMethod(std::move(bm))};
                            pop();
                            push(Value{bmIndex});
                            break;
                        }

                        throw std::runtime_error("undefined property '" + name + "'");
                    }

                    case OpCode::OP_SET_PROPERTY: {
                        if(!std::holds_alternative<InstanceIndex>(peek(1))) {
                            throw std::runtime_error("only instances have fields");
                        }

                        InstanceIndex instIndex{std::get<InstanceIndex>(peek(1))};
                        ObjInstance& instance{instancePool_.getInstance(instIndex)};
                        
                        std::uint8_t nameIndex{*frame->ip++};
                        Value nameVal{currentFunction(*frame).chunk.getConstant(nameIndex)};
                        std::string name{std::visit([](auto&& v) -> std::string {
                            using T = std::decay_t<decltype(v)>;
                            if constexpr(std::is_same_v<T, std::string>) return v;
                            else if constexpr(std::is_same_v<T, std::string_view>) return std::string(v);
                            else throw std::runtime_error("property name must be a string");
                        }, nameVal)};

                        instance.fields[name] = peek(0);
                        Value value{pop()}; // pop the value
                        pop(); // pop the instance
                        push(value); // push the value back (assignment is an expression)
                        break;
                    }

                    case OpCode::OP_METHOD: {
                        std::uint8_t nameIndex{*frame->ip++};
                        Value nameVal{currentFunction(*frame).chunk.getConstant(nameIndex)};
                        std::string name{std::get<std::string>(nameVal)};

                        Value methodVal{peek(0)}; // the closure on top of stack
                        ClosureIndex closureIndex{std::get<ClosureIndex>(methodVal)};

                        Value classVal{peek(1)}; // the class below it
                        ClassIndex clsIndex{std::get<ClassIndex>(classVal)};

                        ObjClass& cls{classPool_.getClass(clsIndex)};
                        cls.methods[name] = closureIndex;

                        pop(); // pop the closure
                        break;
                    }

                    case OpCode::OP_INVOKE: {
                        std::uint8_t nameIndex{*frame->ip++};
                        std::uint8_t argCount{*frame->ip++};

                        Value nameVal{currentFunction(*frame).chunk.getConstant(nameIndex)};
                        std::string name{std::get<std::string>(nameVal)};

                        Value receiver{peek(argCount)};
                        if(!std::holds_alternative<InstanceIndex>(receiver)) {
                            throw std::runtime_error("only instances have methods");
                        }

                        InstanceIndex instIndex{std::get<InstanceIndex>(receiver)};
                        ObjInstance& instance{instancePool_.getInstance(instIndex)};

                        // check fields first (a field could be a closure)
                        auto fieldIt{instance.fields.find(name)};
                        if(fieldIt != instance.fields.end()) {
                            stackTop_[-argCount - 1] = fieldIt->second;
                            if(!callValue(fieldIt->second, argCount)) {
                                return InterpretResult::RUNTIME_ERROR;
                            }
                            frame = &frames_[frameCount_ - 1];
                            break;
                        }

                        // check methods
                        ObjClass& cls{classPool_.getClass(instance.classIndex)};
                        auto methodIt{cls.methods.find(name)};
                        if(methodIt == cls.methods.end()) {
                            throw std::runtime_error("undefined property '" + name + "'");
                        }

                        const ObjClosure& closure{closurePool_.getClosure(methodIt->second)};
                        const ObjFunction& function{funcPool_.getFunction(closure.funcIndex)};

                        if(function.arity != argCount) {
                            throw std::runtime_error("expected " + std::to_string(function.arity) + " arguments but got " + std::to_string(argCount));
                        }

                        CallFrame& newFrame{frames_[frameCount_++]};
                        newFrame.closureIndex = methodIt->second;
                        newFrame.ip = function.chunk.getCode();
                        newFrame.slots = stackTop_ - argCount - 1;
                        frame = &frames_[frameCount_ - 1];
                        break;
                    }

                    case OpCode::OP_INHERIT: {
                        Value superVal{peek(1)};
                        if(!std::holds_alternative<ClassIndex>(superVal)) {
                            throw std::runtime_error("superclass must be a class");
                        }

                        ClassIndex superIndex{std::get<ClassIndex>(superVal)};
                        ObjClass& superclass{classPool_.getClass(superIndex)};

                        Value subVal{peek(0)};
                        ClassIndex subIndex{std::get<ClassIndex>(subVal)};
                        ObjClass& subclass{classPool_.getClass(subIndex)};

                        // copy all methods from superclass to subclass
                        for(auto& [name, method] : superclass.methods) {
                            subclass.methods[name] = method;
                        }

                        pop(); // pop subclass
                        break;
                    }

                    case OpCode::OP_GET_SUPER: {
                        std::uint8_t nameIndex{*frame->ip};
                        Value nameVal{currentFunction(*frame).chunk.getConstant(nameIndex)};
                        std::string name{std::get<std::string>(nameVal)};

                        Value superVal{pop()}; // pop the superclass
                        ClassIndex superIndex{std::get<ClassIndex>(superVal)};
                        ObjClass&  superclass{classPool_.getClass(superIndex)};

                        auto methodIt{superclass.methods.find(name)};
                        if(methodIt == superclass.methods.end()) {
                            throw std::runtime_error("undefined property '" + name + "'");
                        }

                        // the instance (this) is still on the stack
                        InstanceIndex instIndex{std::get<InstanceIndex>(peek(0))};

                        ObjBoundMethod bm;
                        bm.receiver = instIndex;
                        bm.method = methodIt->second;
                        BoundMethodIndex bmIndex{boundMethodPool_.addBoundMethod(std::move(bm))};

                        pop(); // pop the instance
                        push(Value{bmIndex});
                        break;
                    }

                    case OpCode::OP_SUPER_INVOKE: {
                        std::uint8_t nameIndex{*frame->ip};
                        std::uint8_t argCount{*frame->ip};

                        Value nameVal{currentFunction(*frame).chunk.getConstant(nameIndex)};
                        std::string name{std::get<std::string>(nameVal)};

                        Value superVal{pop()}; // pop the superclass
                        ClassIndex superIndex{std::get<ClassIndex>(superVal)};
                        ObjClass& superclass{classPool_.getClass(superIndex)};

                        auto methodIt{superclass.methods.find(name)};
                        if(methodIt == superclass.methods.end()) {
                            throw std::runtime_error("undefined property '" + name + "'");
                        }

                        const ObjClosure& closure{closurePool_.getClosure(methodIt->second)};
                        const ObjFunction& function{funcPool_.getFunction(closure.funcIndex)};

                        if(function.arity != argCount) {
                            throw std::runtime_error("expected " + std::to_string(function.arity) + " arguments but got " + std::to_string(argCount));
                        }

                        CallFrame& newFrame{frames_[frameCount_++]};
                        newFrame.closureIndex = methodIt->second;
                        newFrame.ip = function.chunk.getCode();
                        newFrame.slots = stackTop_ - argCount - 1;
                        frame = &frames_[frameCount_ - 1];
                        break;
                    }

                    case OpCode::OP_DEFINE_GLOBAL_IMMUT: {
                        const std::size_t constantIndex{*frame->ip++};
                        Value identifier{currentFunction(*frame).chunk.getConstant(constantIndex)};
                        std::string_view variableName{extractVariableName(identifier)};
                        std::string_view internedName{strPool_.intern(variableName)};
                        globalVariables_[internedName] = peek(0);
                        immutableGlobals_.insert(std::string(internedName));
                        pop();
                        break;
                    }

                    case OpCode::OP_DEFINE_GLOBAL_IMMUT_LONG: {
                        const std::size_t constantIndex{readConstantIndexLong(frame->ip)};
                        Value identifier{currentFunction(*frame).chunk.getConstant(constantIndex)};
                        std::string_view variableName{extractVariableName(identifier)};
                        std::string_view internedName{strPool_.intern(variableName)};
                        globalVariables_[internedName] = peek(0);
                        immutableGlobals_.insert(std::string(internedName));
                        pop();
                        break;
                    }

                    case OpCode::OP_GET_INDEX: {
                        Value indexVal{pop()};
                        Value arrayVal{pop()};

                        if(!std::holds_alternative<ArrayIndex>(arrayVal)) {
                            throw std::runtime_error("can only index into arrays");
                        }

                        if(!std::holds_alternative<int>(indexVal)) {
                            throw std::runtime_error("array index must be an integer");
                        }

                        ArrayIndex arrIndex{std::get<ArrayIndex>(arrayVal)};
                        int index{std::get<int>(indexVal)};
                        ObjArray& arr{arrayPool_.getArray(arrIndex)};

                        if(index < 0 || static_cast<std::size_t>(index) >= arr.elements.size()) {
                            throw std::runtime_error("array index out of bounds");
                        }

                        push(arr.elements[static_cast<std::size_t>(index)]);
                        break;
                    }

                    case OpCode::OP_SET_INDEX: {
                        Value value{pop()};
                        Value indexVal{pop()};
                        Value arrayVal{pop()};

                        if(!std::holds_alternative<ArrayIndex>(arrayVal)) {
                            throw std::runtime_error("can only index into arrays");
                        }

                        if(!std::holds_alternative<int>(indexVal)) {
                            throw std::runtime_error("array index must be an integer");
                        }

                        ArrayIndex arrIndex{std::get<ArrayIndex>(arrayVal)};
                        int index{std::get<int>(indexVal)};
                        ObjArray& arr{arrayPool_.getArray(arrIndex)};

                        if(index < 0 || static_cast<std::size_t>(index) >= arr.elements.size()) {
                            throw std::runtime_error("array index out of bounds");
                        }

                        arr.elements[static_cast<std::size_t>(index)] = value;
                        push(value); // assignment is an expression
                        break;
                    }

                    case OpCode::OP_ARRAY: {
                        std::uint8_t count{*frame->ip++};
                        ObjArray arr;
                        arr.elements.resize(count);

                        // elements are on the stack in order, top is last element
                        for(std::size_t i{count}; i > 0; i--) {
                            arr.elements[i - 1] = pop();
                        }

                        ArrayIndex arrIndex{arrayPool_.addArray(std::move(arr))};
                        push(Value{arrIndex});
                        break;
                    }

                    case OpCode::OP_ARRAY_LEN: {
                        Value arrayVal{pop()};

                        if(!std::holds_alternative<ArrayIndex>(arrayVal)) {
                            throw std::runtime_error("len() argument must be an array");
                        }

                        ArrayIndex arrIndex{std::get<ArrayIndex>(arrayVal)};
                        ObjArray& arr{arrayPool_.getArray(arrIndex)};
                        push(Value{static_cast<int>(arr.elements.size())});
                        break;
                    }

                    case OpCode::OP_ARRAY_PUSH: {
                        Value val{pop()};
                        Value arrayVal{pop()};

                        if(!std::holds_alternative<ArrayIndex>(arrayVal)) {
                            throw std::runtime_error("can only push to arrays");
                        }

                        ArrayIndex arrIndex{std::get<ArrayIndex>(arrayVal)};
                        ObjArray& arr{arrayPool_.getArray(arrIndex)};
                        arr.elements.push_back(val);
                        push(val); // push the value back (array.push() is an expression)
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
                        closeUpvalues(frame->slots);
                        frameCount_--;
                        if(frameCount_ == 0) {
                            pop();
                            return InterpretResult::OK;
                        }
                        stackTop_ = frame->slots;
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
        
        ObjClosure scriptClosure;
        scriptClosure.funcIndex = funcIndex;
        ClosureIndex closureIndex{closurePool_.addClosure(std::move(scriptClosure))};

        push(Value{closureIndex});
        CallFrame& frame{frames_[frameCount_++]};
        frame.closureIndex = closureIndex;
        frame.ip = funcPool_.getFunction(funcIndex).chunk.getCode();
        frame.slots = stack_.data();

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

        if(std::holds_alternative<int>(a) && std::holds_alternative<int>(b)) {
            int aVal{std::get<int>(a)};
            int bVal{std::get<int>(b)};
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
        } else if(std::holds_alternative<double>(a) && std::holds_alternative<double>(b)) {
            double aVal{std::get<double>(a)};
            double bVal{std::get<double>(b)};
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
        } else if(std::holds_alternative<std::string>(a) && std::holds_alternative<std::string>(b)) {
            const std::string& aVal{std::get<std::string>(a)};
            const std::string& bVal{std::get<std::string>(b)};
            switch(op) {
                case BinaryOp::ADD:         {push(Value{std::string{aVal} + std::string{bVal}});break;}
                case BinaryOp::EQUAL:       {push(Value{aVal == bVal});break;}
                case BinaryOp::GREATER:     {push(Value{aVal > bVal});break;}
                case BinaryOp::LESS:        {push(Value{aVal < bVal});break;}
                default:
                    throw std::runtime_error("invalid operator for strings");
            }
        } else if(std::holds_alternative<std::string_view>(a) && std::holds_alternative<std::string_view>(b)) {
            std::string_view aVal{std::get<std::string_view>(a)};
            std::string_view bVal{std::get<std::string_view>(b)};
            switch(op) {
                case BinaryOp::ADD:         {push(Value{std::string{aVal} + std::string{bVal}});break;}
                case BinaryOp::EQUAL:       {push(Value{aVal == bVal});break;}
                case BinaryOp::GREATER:     {push(Value{aVal > bVal});break;}
                case BinaryOp::LESS:        {push(Value{aVal < bVal});break;}
                default:
                    throw std::runtime_error("invalid operator for strings");
            }
        }  else {
                    throw std::runtime_error("operands must be of same type for binary operations: " + 
                    std::to_string(a.index()) + " and " + std::to_string(b.index()));
            }
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

        if(std::holds_alternative<ClassIndex>(callee)) {
            ClassIndex clsIndex{std::get<ClassIndex>(callee)};
            ObjInstance instance;
            instance.classIndex = clsIndex;
            InstanceIndex instIndex{instancePool_.addInstance(std::move(instance))};
            // replace the class on the stack with the new instance
            stackTop_[-argCount - 1] = Value{instIndex};

            // look for an init method
            ObjClass& cls{classPool_.getClass(clsIndex)};
            auto initIt{cls.methods.find("init")};
            if(initIt != cls.methods.end()) {
                const ObjClosure& closure{closurePool_.getClosure(initIt->second)};
                const ObjFunction& function{funcPool_.getFunction(closure.funcIndex)};

                if(function.arity != argCount) {
                    throw std::runtime_error("expected " + std::to_string(function.arity) + " arguments but got " + std::to_string(argCount));
                }

                CallFrame& newFrame{frames_[frameCount_++]};
                newFrame.closureIndex = initIt->second;
                newFrame.ip = function.chunk.getCode();
                newFrame.slots = stackTop_ - argCount - 1;
            } else if(argCount != 0) {
                throw std::runtime_error("expected 0 arguments but got " + std::to_string(argCount));
            }


            return true;
        }

        if(std::holds_alternative<BoundMethodIndex>(callee)) {
            BoundMethodIndex bmIndex{std::get<BoundMethodIndex>(callee)};
            const ObjBoundMethod& bm{boundMethodPool_.getBoundMethod(bmIndex)};

            // put the reciever in slot 0 where 'this' will be
            stackTop_[-argCount - 1] = Value{bm.receiver};

            const ObjClosure& closure{closurePool_.getClosure(bm.method)};
            const ObjFunction& function{funcPool_.getFunction(closure.funcIndex)};

            if(function.arity != argCount) {
                throw std::runtime_error("expected " + std::to_string(function.arity) + " arguments but got " + std::to_string(argCount));
            }

            if(frameCount_ >= FRAME_MAX) {
                throw std::runtime_error("stack overflow");
            }

            CallFrame& newFrame{frames_[frameCount_++]};
            newFrame.closureIndex = bm.method;
            newFrame.ip = function.chunk.getCode();
            newFrame.slots = stackTop_ - argCount - 1;
            return true;
        }

        if(std::holds_alternative<ClosureIndex>(callee)) {
            const ClosureIndex closureIndex{std::get<ClosureIndex>(callee)};
            const ObjClosure& closure{closurePool_.getClosure(closureIndex)};
            const ObjFunction& function{funcPool_.getFunction(closure.funcIndex)};
            
            if(function.arity != argCount) {
                throw std::runtime_error("expected " + std::to_string(function.arity) + " arguments but got " + std::to_string(argCount));
            }

            if(frameCount_ >= FRAME_MAX) {
                throw std::runtime_error("stack overflow");
            }

            CallFrame& newFrame{frames_[frameCount_++]};
            newFrame.closureIndex = closureIndex;
            newFrame.ip = function.chunk.getCode();
            newFrame.slots = stackTop_ - argCount - 1;
            return true;
        }

        throw std::runtime_error("can only call functions");
    }

    const ObjFunction& VM::currentFunction(const CallFrame &frame) {
        const ObjClosure& closure{closurePool_.getClosure(frame.closureIndex)};
        return funcPool_.getFunction(closure.funcIndex);
    }
    
    void VM::runtimeError(std::string_view errorMessage) {
        fprintf(stderr, "%s\n", errorMessage.data());

        for(std::size_t i{frameCount_}; i > 0; i--) {
            const CallFrame& frame{frames_[i - 1]};
            const ObjClosure& closure{closurePool_.getClosure(frame.closureIndex)};
            const ObjFunction& objFunc{funcPool_.getFunction(closure.funcIndex)};
            std::size_t offset{static_cast<std::size_t>(frame.ip - objFunc.chunk.getCode())};
            fprintf(stderr, "[line %d] in ", objFunc.chunk.getLine(offset));

            if(objFunc.name.empty()) {
                fprintf(stderr, "script\n");
            } else {
                fprintf(stderr, "%s()\n", objFunc.name.c_str());
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
    
    UpvalueIndex VM::captureUpvalue(Value *local) {
        UpvalueIndex prev{SIZE_MAX};
        UpvalueIndex current{openUpvalueHead_};

        // walk the list looking for an existing upvalue or the right insertion point
        while (current.index != SIZE_MAX) {
            ObjUpValue& uv{upvaluePool_.getUpvalue(current)};
            if(uv.location == local) {
                return current;
            }

            if(uv.location < local) {
                break;
            }

            prev = current;
            current = uv.next;
        }

        // create new upvalue
        ObjUpValue newUpvalue;
        newUpvalue.location = local;
        newUpvalue.next = current;
        UpvalueIndex newIndex{upvaluePool_.addUpValue(std::move(newUpvalue))};
    
        if(prev.index == SIZE_MAX) {
            openUpvalueHead_ = newIndex;
        } else {
            upvaluePool_.getUpvalue(prev).next = newIndex;
        }

        return newIndex;
    }
    
    void VM::closeUpvalues(Value *last) {
        while(openUpvalueHead_.index != SIZE_MAX) {
            ObjUpValue& uv{upvaluePool_.getUpvalue(openUpvalueHead_)};
            if(uv.location < last) break;

            uv.closed = *uv.location;
            uv.location = &uv.closed;
            openUpvalueHead_ = uv.next;
        }
    }
}