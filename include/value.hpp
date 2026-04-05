#pragma once
#include <cstdio>
#include <string>
#include <variant>
#include <stdexcept>
#include <string_view>

namespace pegasus {
    using Value = std::variant<int, double, bool, std::string, std::string_view, std::monostate>;

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
            } else {
                throw std::runtime_error("operand must be a number");
            }
        }, value);
    }
    
    inline Value notValue(const Value& value) {
        return std::visit([](auto&& v) -> Value {
            using T = std::decay_t<decltype(v)>;
            if constexpr(std::is_arithmetic_v<T>) {
                return Value{!v};
            } else {
                throw std::runtime_error("operand must be a number");
            }
        }, value);
    }
}