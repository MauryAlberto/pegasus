#pragma once
#include <cstdio>
#include <variant>
#include <stdexcept>

namespace pegasus {
    using Value = std::variant<double>;

    inline void printValue(const Value& value) {
        std::visit([](auto v) {
            using T = decltype(v);
            if constexpr(std::is_same_v<T, double>) {
                printf("%.2f\n", v);
            } else {
                printf("unknown value type\n");
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
}