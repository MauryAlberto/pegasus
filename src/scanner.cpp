#include "scanner.hpp"

namespace pegasus {
    Token Scanner::scanToken() {
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

    bool Scanner::isAtEnd() {
        return *current_ == '\0';
    }
    
    char Scanner::advance() {
        current_++;
        return current_[-1];
    }

    bool Scanner::match(char expected) {
        if(isAtEnd() || *current_ != expected) return false;
        current_++;
        return true;
    }

    char Scanner::peekNext() {
        if(isAtEnd()) return '\0';
        return current_[1];
    }

    void Scanner::skipWhiteSpace() {
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

    char Scanner::peek() {
        return *current_;
    }

    Token Scanner::makeToken(TokenType type) {
        if(type == TokenType::STRING) {
            return Token{
                type,                                                 // example: "hello world" must become hello world
                std::string_view{start_ + 1, static_cast<std::size_t>(current_ - start_ - 2)}, 
                line_
            };
        }

        return Token{
            type,
            std::string_view{start_, static_cast<std::size_t>(current_ - start_)},
            line_
        };
    }

    Token Scanner::errorToken(std::string_view message) {
        return Token{
            TokenType::ERROR,
            message,
            line_
        };
    }

    Token Scanner::string() {
        while(peek() != '"' && !isAtEnd()) {
            if(peek() == '\n') line_++;
            advance();
        }

        if(isAtEnd()) return errorToken("unterminated string");
        advance();
        return makeToken(TokenType::STRING);
    }

    Token Scanner::identifier() {
        while(std::isalpha(peek()) || *current_ == '_' || std::isdigit(peek())) advance();
        return makeToken(identifierType());
    }

    Token Scanner::number() {
        while(std::isdigit(peek())) advance();

        if(peek() == '.' && std::isdigit(peekNext())) {
            advance();
            while(std::isdigit(peek())) advance();
        }

        return makeToken(TokenType::NUMBER);
    }

    TokenType Scanner::matchKeyword(std::string_view rest, TokenType type) {
        std::string_view keyword{start_, static_cast<size_t>(current_ - start_)};
        if(keyword == rest) return type;
        return TokenType::IDENTIFIER;
    }

    TokenType Scanner::identifierType() {
        switch(start_[0]) {
            case 'a': return matchKeyword("and", TokenType::AND);
            case 'c': return matchKeyword("class", TokenType::CLASS);
            case 'e': return matchKeyword("else", TokenType::ELSE);
            case 'i':
                switch(start_[1]) {
                    case 'f': return matchKeyword("if", TokenType::IF);
                    case 'm': return matchKeyword("immut", TokenType::IMMUT);
                }
                break;
            case 'm': return matchKeyword("mut", TokenType::MUT);
            case 'n': return matchKeyword("nil", TokenType::NIL);
            case 'o': return matchKeyword("or", TokenType::OR);
            case 'p': return matchKeyword("print", TokenType::PRINT);
            case 'r': return matchKeyword("return", TokenType::RETURN);
            case 's': return matchKeyword("super", TokenType::SUPER);
            case 'w': return matchKeyword("while", TokenType::WHILE);
            case 'f':
                switch(start_[1]) {
                    case 'a': return matchKeyword("false", TokenType::FALSE);
                    case 'o': return matchKeyword("for", TokenType::FOR);
                    case 'n': return matchKeyword("fn", TokenType::FN);
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
}