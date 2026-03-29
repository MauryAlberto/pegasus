#pragma once
#include <string>
#include <cctype>

namespace pegasus {

    enum class TokenType {
        // single-character tokens
        LEFT_PAREN, RIGHT_PAREN,
        LEFT_BRACE, RIGHT_BRACE,
        COMMA, DOT, MINUS, PLUS,
        SEMICOLON, SLASH, STAR,
        // one or two character tokens
        NOT, NOT_EQUAL,
        EQUAL, EQUAL_EQUAL,
        GREATER, GREATER_EQUAL,
        LESS, LESS_EQUAL,
        // literals
        IDENTIFIER, STRING, NUMBER,
        // keywords
        TRUE, FALSE, IF, ELSE,
        AND, OR, FOR, WHILE,
        FUN, CLASS, PRINT, RETURN,
        VAR, THIS, NIL, SUPER,

        ERROR, EOF_,
        TOKEN_SIZE
    };

    struct Token {
        TokenType type_;
        std::string_view lexeme_;
        int line_;
    };

    class Scanner {
        public:
            Scanner() = delete;
            explicit Scanner(std::string_view source) : start_{source.data()}, current_{source.data()}, line_{1} {}
            Token scanToken();

        private:
            const char* start_{nullptr};
            const char* current_{nullptr};
            int line_{-1};

            bool isAtEnd();            
            char advance();
            bool match(char expected);
            char peekNext();
            void skipWhiteSpace();
            char peek();
            Token makeToken(TokenType type);
            Token errorToken(std::string_view message);
            Token string();
            Token identifier();
            Token number();
            TokenType matchKeyword(std::string_view rest, TokenType type);
            TokenType identifierType();
    };
}
