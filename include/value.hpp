#pragma once
#include <cstdio>
#include <variant>

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