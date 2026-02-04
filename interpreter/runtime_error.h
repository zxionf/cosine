#pragma once

#include <string>
#include "token.h"

namespace xel::runtime_error
{
    inline bool hadError = false;
    inline bool hadRuntimeError = false;

    inline void report(int line, const std::string& where, const std::string& message) {
        std::printf("runtime error: [line %d] Error%s: %s\n", line, where.c_str(), message.c_str());
        hadError = true;
    }
    inline void error(int line, const std::string &message) {
        report(line, "", message);
    }

    inline void error(Token token, const std::string& message) {
        if (token.get_type() == Token::Type::TOKEN_EOF)
            report(token.get_line(), " at end", message);
        else 
            report(token.get_line(), " at '" + token.get_lexeme() + "'", message);
    }

    // void runtime_error(std::runtime_error error) {
    //     System.err.println(error.getMessage() +
    //         "\n[line " + error.token.line + "]");
    //     std::printf("", line, error.);
    //     hadRuntimeError = true;
    // }
}