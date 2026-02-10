#pragma once

#include "token.h"
#include <stdexcept>

namespace xel
{

    inline bool hadError = false;
    inline bool hadRuntimeError = false;

    namespace error
    {
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
    }
    
    namespace runtime_error
    {
        inline void error(const Token& token, const std::string& message) {
            std::printf("xel runtime error: [line %d] %s: %s\n", token.get_line(), token.get_lexeme(), message);
            hadRuntimeError = true;
        }

        inline std::runtime_error error_(const Token& token, const std::string& message) {
            std::runtime_error err("xel runtime error: [line " + std::to_string(token.get_line()) + "] " + token.get_lexeme() + ": " + message);
            hadRuntimeError = true;
            return err;
        }
    }
}