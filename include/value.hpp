#pragma once
#include <cstdio>
#include <string>
#include <variant>
#include <stdexcept>
#include <string_view>
#include "function_index.hpp"

namespace pegasus {
    struct NativeFunction;
    using Value = std::variant<int, double, bool, std::string, std::string_view, std::monostate, FunctionIndex, NativeFunction>;
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
                printf("%zu", v.index_);
            } else if constexpr(std::is_same_v<T, NativeFunction>) {
                printf("<native fn>");
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
            } else {
                throw std::runtime_error("unhandled type in isFalsey");
            }
        }, value);
    }

    inline Value notValue(const Value& value) {
        return Value{isFalsey(value)};
    }
}