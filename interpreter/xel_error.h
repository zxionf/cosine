#pragma once

#include "token.h"

namespace xel
{

    inline bool hadError = false;
    inline bool hadRuntimeError = false;

    namespace error
    {
        inline void report(int line, const std::string& where, const std::string& message) {
            std::printf("error: [line %d] %s: %s\n", line, where.c_str(), message.c_str());
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
            std::printf("runtime error: [line %d] %s: %s\n", token.get_line(), token.get_lexeme(), message);
            hadRuntimeError = true;
        }
    }
}