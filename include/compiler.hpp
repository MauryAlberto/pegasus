#pragma once
#include <cstdio>
#include <string_view>
#include "scanner.hpp"

namespace pegasus {
    void compile(std::string_view source) {
        pegasus::Scanner scanner{source};
        int line{-1};

        while(true) {
            Token token{scanner.scanToken()};
            if(token.line != line) {
                printf("%4d ", token.line);
                line = token.line;
            } else {
                printf("   | ");
            }

            printf("%2d '%.*s'", static_cast<int>(token.type), static_cast<int>(token.lexeme.size()), token.lexeme.data());

            if(token.type == TokenType::EOF_) break;
        }
    }
}