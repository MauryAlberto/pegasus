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
                skipWhiteSpace();
                start_ = current_;

                if(isAtEnd()) return makeToken(TokenType::EOF_);

                char c = advance();
                if(std::isalpha(c) || c == '_') return identifier();
                if(std::isdigit(c)) return number();

                switch(c) {
                    case '(': return makeToken(TokenType::LEFT_PAREN);
                    case ')': return makeToken(TokenType::RIGHT_PAREN);
                    case '{': return makeToken(TokenType::LEFT_BRACE);
                    case '}': return makeToken(TokenType::RIGHT_BRACE);
                    case ';': return makeToken(TokenType::SEMICOLON);
                    case ',': return makeToken(TokenType::COMMA);
                    case '.': return makeToken(TokenType::DOT);
                    case '-': return makeToken(TokenType::MINUS);
                    case '+': return makeToken(TokenType::PLUS);
                    case '/': return makeToken(TokenType::SLASH);
                    case '*': return makeToken(TokenType::STAR);
                    case '!': return makeToken(
                        match('=') ? TokenType::NOT_EQUAL :TokenType::NOT);
                    case '=': return makeToken(
                        match('=') ? TokenType::EQUAL_EQUAL :TokenType::EQUAL);
                    case '<': return makeToken(
                        match('=') ? TokenType::LESS_EQUAL :TokenType::LESS);
                    case '>': return makeToken(
                        match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER);
                    case '"': return string();
                }

                return errorToken("unexpected character");
            }

        private:
            const char* start_{nullptr};
            const char* current_{nullptr};
            int line_{-1};

            bool isAtEnd() {
                return *current_ == '\0';
            }
            
            char advance() {
                current_++;
                return current_[-1];
            }

            bool match(char expected) {
                if(isAtEnd() || *current_ != expected) return false;
                current_++;
                return true;
            }

            char peekNext() {
                if(isAtEnd()) return '\0';
                return current_[1];
            }

            void skipWhiteSpace() {
                while(true) {
                    char c{peek()};
                    if(std::isspace(c)) {
                        if(c == '\n') line_++;
                        advance();
                    } else if(c == '/' && peekNext() == '/') {
                        while(peek() != '\n' && !isAtEnd()) advance();
                    } else {
                        return;
                    }
                }
            }

            char peek() {
                return *current_;
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

            Token string() {
                while(peek() != '"' && !isAtEnd()) {
                    if(peek() == '\n') line_++;
                    advance();
                }

                if(isAtEnd()) return errorToken("unterminated string");
                advance();
                return makeToken(TokenType::STRING);
            }

            Token identifier() {
                while(std::isalpha(peek()) || *current_ == '_' || std::isdigit(peek())) advance();
                return makeToken(identifierType());
            }

            Token number() {
                while(std::isdigit(peek())) advance();

                if(peek() == '.' && std::isdigit(peekNext())) {
                    advance();
                    while(std::isdigit(peek())) advance();
                }

                return makeToken(TokenType::NUMBER);
            }

            TokenType matchKeyword(std::string_view rest, TokenType type) {
                std::string_view keyword{start_, current_ - start_};
                if(keyword == rest) return type;
                return TokenType::IDENTIFIER;
            }

            TokenType identifierType() {
                switch(start_[0]) {
                    case 'a': return matchKeyword("and", TokenType::AND);
                    case 'c': return matchKeyword("class", TokenType::CLASS);
                    case 'e': return matchKeyword("else", TokenType::ELSE);
                    case 'i': return matchKeyword("if", TokenType::IF);
                    case 'n': return matchKeyword("nil", TokenType::NIL);
                    case 'o': return matchKeyword("or", TokenType::OR);
                    case 'p': return matchKeyword("print", TokenType::PRINT);
                    case 'r': return matchKeyword("return", TokenType::RETURN);
                    case 's': return matchKeyword("super", TokenType::SUPER);
                    case 'v': return matchKeyword("var", TokenType::VAR);
                    case 'w': return matchKeyword("while", TokenType::WHILE);
                    case 'f':
                        switch(start_[1]) {
                            case 'a': return matchKeyword("false", TokenType::FALSE);
                            case 'o': return matchKeyword("for", TokenType::FOR);
                            case 'u': return matchKeyword("fun", TokenType::FUN);
                        }
                        break;
                    case 't':
                        switch(start_[1]) {
                            case 'h': return matchKeyword("this", TokenType::THIS);
                            case 'r': return matchKeyword("true", TokenType::TRUE);
                        }
                        break;
                    default:
                        return TokenType::IDENTIFIER;
                }
                return TokenType::IDENTIFIER;
            }
    };
}
