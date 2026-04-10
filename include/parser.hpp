#pragma once
#include <string_view>
#include "scanner.hpp"

namespace pegasus {
    class Parser {
        public:
            Parser() = delete;
            explicit Parser(Scanner& scanner) : scanner_{scanner} {}

            void advance();
            void consume(TokenType expectedType, std::string_view message);
            int previousLine() const;
            const Token& previousToken() const;
            const Token& currentToken() const;
            bool hadError() const;
            bool isInPanicMode() const;
            void resetPanicMode();
            void errorAt(const Token& token, std::string_view message);
            void errorAtCurrent(std::string_view message);
            void error(std::string_view message);

        private:
            Scanner& scanner_;
            Token current_{};
            Token previous_{};
            bool hadError_{false};
            bool panicMode_{false};
    };
}