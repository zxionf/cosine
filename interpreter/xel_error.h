#pragma once

#include "token.h"
#include <stdexcept>

namespace xel
{

    inline bool hadError = false;
    inline bool hadRuntimeError = false;

    class runtime_error : public std::exception
    {
        public:
            std::string message;
            explicit runtime_error(std::string message) :message(std::move(message)) {}
    };

    namespace error
    {
        // syntax error
        inline void report(int line, const std::string& where, const std::string& message) {
            std::printf("xel error: [line %d] %s: %s\n", line, where.c_str(), message.c_str());
            hadError = true;
        }

        inline void error(int line, const std::string &message) {
            report(line, "", message);
        }

        inline void error(const Token& token, const std::string& message) {
            if (token.get_type() == Token::Type::TOKEN_EOF)
                report(token.get_line(), " at end", message);
            else 
                report(token.get_line(), " at '" + token.get_lexeme() + "'", message);
        }

        // runtime error
        inline void _error(const Token& token, const std::string& message) {
            std::printf("xel runtime error: [line %d] %s: %s\n", token.get_line(), token.get_lexeme(), message);
            hadRuntimeError = true;
        }

        inline runtime_error error_(const Token& token, const std::string& message) {
            xel::runtime_error err("xel runtime error: [line " + std::to_string(token.get_line()) + "] " + token.get_lexeme() + ": " + message);
            hadRuntimeError = true;
            return err;
        }

        class Return : public std::exception
        {
            public:
                var value;
                explicit Return(var value) :value(std::move(value)) {}
        };
    }
}