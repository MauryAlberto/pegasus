#pragma once
#include <cstdio>
#include <variant>
#include <stdexcept>

namespace pegasus {
    using Value = std::variant<int, double, bool>;

    inline void printValue(const Value& value) {
        std::visit([](auto v) {
            using T = decltype(v);
            if constexpr(std::is_same_v<T, int>) {
                printf("%d", v);
            } else if constexpr(std::is_same_v<T, double>) {
                printf("%g", v);
            } else if constexpr(std::is_same_v<T, bool>) {
                printf("%s", v ? "true" : "false");
            } else {
                throw std::runtime_error("unknown value type");
            }
        }, value);
    }

    inline Value negateValue(const Value& value) {
        return std::visit([](auto v) -> Value {
            using T = decltype(v);
            if constexpr(std::is_arithmetic_v<T>) {
                return Value{-v};
            } else {
                throw std::runtime_error("operand must be a number");
            }
        }, value);
    }
    
    inline Value notValue(const Value& value) {
        return std::visit([](auto v) -> Value {
            return Value{!v};
        }, value);
    }
}