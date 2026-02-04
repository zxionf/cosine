#pragma once

#include <string>
#include "token.h"
using namespace xel;

namespace xel::runtime_error
{
    bool hadError = false;

    void error(int line, const std::string &message) {
        report(line, "", message);
    }

    void report(int line, const std::string& where, const std::string& message) {
        std::printf("runtime error: [line %d] Error%s: %s\n", line, where.c_str(), message.c_str());
        hadError = true;
    }

    void error(Token token, const std::string& message) {
        if (token.get_type() == Token::Type::TOKEN_EOF)
            report(token.get_line(), " at end", message);
        else 
            report(token.get_line(), " at '" + token.get_lexeme() + "'", message);
    }
}