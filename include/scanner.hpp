#pragma once
#include <string>

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

        ERROR, EOF_
    };

    struct Token {
        TokenType type;
        std::string_view lexeme;
        int line;
    };

    class Scanner {
        public:
            Scanner() = delete;
            explicit Scanner(std::string_view source) : start_{source.data()}, current_{source.data()}, line_{1} {}

            Token scanToken() {
                start_ = current_;

                if(isAtEnd()) return makeToken(TokenType::EOF_);

                return errorToken("unexpected character");
            }

        private:
            const char* start_{nullptr};
            const char* current_{nullptr};
            int line_{-1};

            bool isAtEnd() {
                return *current_ == '\0';
            }

            Token makeToken(TokenType type) {
                return Token{
                    type,
                    std::string_view{start_, static_cast<std::size_t>(current_ - start_)},
                    line_
                };
            }

            Token errorToken(std::string_view message) {
                return Token{
                    TokenType::ERROR,
                    message,
                    line_
                };
            }
    };
}
