#pragma once
#include <cstdio>
#include <string>
#include <variant>
#include <stdexcept>
#include <string_view>
#include "function_index.hpp"
#include "closure_index.hpp"
#include "class_index.hpp"
#include "instance_index.hpp"
#include "bound_method_index.hpp"
#include "array_index.hpp"

namespace pegasus {
    struct NativeFunction;
    using Value = std::variant<int, double, bool, std::string, std::string_view,
                            std::monostate, FunctionIndex, NativeFunction, ClosureIndex,
                            ClassIndex, InstanceIndex, BoundMethodIndex, ArrayIndex>;
    using NativeFn = Value(*)(std::size_t argCount, Value* args);
    struct NativeFunction {
        NativeFn function_;
    };

    inline void printValue(const Value& value) {
        std::visit([](auto&& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr(std::is_same_v<T, int>) {
                printf("%d", v);
            } else if constexpr(std::is_same_v<T, double>) {
                printf("%g", v);
            } else if constexpr(std::is_same_v<T, bool>) {
                printf("%s", v ? "true" : "false");
            } else if constexpr(std::is_same_v<T, std::string_view>) {
                printf("%.*s", static_cast<int>(v.size()), v.data());
            } else if constexpr(std::is_same_v<T, std::string>){
                printf("%s", v.c_str());
            } else if constexpr(std::is_same_v<T, std::monostate>) {
                printf("nil");
            } else if constexpr(std::is_same_v<T, FunctionIndex>) {
                printf("%zu", v.index);
            } else if constexpr(std::is_same_v<T, NativeFunction>) {
                printf("<native fn>");
            } else if constexpr (std::is_same_v<T, ClosureIndex>) {
                printf("<closure>");
            } else if constexpr(std::is_same_v<T, ClassIndex>) {
                printf("<class>");
            } else if constexpr(std::is_same_v<T, InstanceIndex>) {
                printf("<instance>");
            } else if constexpr(std::is_same_v<T, BoundMethodIndex>) {
                printf("<bound method>");
            } else if constexpr(std::is_same_v<T, ArrayIndex>)  {
                printf("<array>");
            } else {
                throw std::runtime_error("unknown value type");
            }
        }, value);
    }

    inline Value negateValue(const Value& value) {
        return std::visit([](auto&& v) -> Value {
            using T = std::decay_t<decltype(v)>;
            if constexpr(std::is_arithmetic_v<T>) {
                return Value{-v};
            }  else if constexpr(std::is_same_v<T, NativeFunction>) {
                throw std::runtime_error("cannot negate a native function");
            } else if constexpr(std::is_same_v<T, FunctionIndex>) {
                throw std::runtime_error("cannot negate a function index");
            } else if constexpr(std::is_same_v<T, ClosureIndex>) {
                throw std::runtime_error("cannote negate a closure index");
            }  else if constexpr(std::is_same_v<T, ClassIndex>) {
                throw std::runtime_error("cannot negate a class index");
            } else if constexpr(std::is_same_v<T, InstanceIndex>) {
                throw std::runtime_error("cannote negate an instance index");
            }  else if constexpr(std::is_same_v<T, BoundMethodIndex>) {
                throw std::runtime_error("cannote negate a bound method index");
            }  else if constexpr(std::is_same_v<T, ArrayIndex>) {
                throw std::runtime_error("cannote negate an array index");
            } else {
                throw std::runtime_error("operand must be a number");
            }
        }, value);
    }

    inline bool isFalsey(const Value& value) {
        return std::visit([](auto&& v) -> bool {
            using T = std::decay_t<decltype(v)>;
            if constexpr(std::is_same_v<T, std::monostate>) {
                return true;
            } else if constexpr(std::is_same_v<T, bool>) {
                return !v;
            } else if constexpr(std::is_arithmetic_v<T>) {
                return v == 0;
            } else if constexpr(std::is_same_v<T, std::string_view> || std::is_same_v<T, std::string>) {
                return v.empty();
            } else if constexpr(std::is_same_v<T, NativeFunction>) {
                throw std::runtime_error("native functions are neither true or false");
            } else if constexpr(std::is_same_v<T, FunctionIndex>) {
                throw std::runtime_error("function index is neither true or false");
            } else if constexpr(std::is_same_v<T, ClosureIndex>) {
                throw std::runtime_error("closure index is neither true or false");
            }  else if constexpr(std::is_same_v<T, ClassIndex>) {
                throw std::runtime_error("class index is neither true or false");
            } else if constexpr(std::is_same_v<T, InstanceIndex>) {
                throw std::runtime_error("instance index is neither true or false");
            }  else if constexpr(std::is_same_v<T, BoundMethodIndex>) {
                throw std::runtime_error("bound method index is neither true or false");
            }  else if constexpr(std::is_same_v<T, ArrayIndex>) {
                throw std::runtime_error("array index is neither true or false");
            } else {
                throw std::runtime_error("unhandled type in isFalsey");
            }
        }, value);
    }

    inline Value notValue(const Value& value) {
        return Value{isFalsey(value)};
    }
}