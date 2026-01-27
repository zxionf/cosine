#pragma once
#include "token.h"
using namespace xel::token;

namespace xel
{
    namespace lexer
    {
        class Lexer
        {
            public:
                Lexer() = default;
                Lexer(const std::string& file);
                Lexer(const char* buffer, size_t size);
                ~Lexer() = default;

                Token next_token();
            private:
                void skip_whitespace();
                void read_char();
                void unread_char();
                bool is_digit(char ch);
                std::string read_number();
                Token new_token(Token::Type type, const std::string& literal);
            private:
                std::string input;
                unsigned int inputlen;
                char ch;
                int pos;
                int next_pos;
        };
    } 
}