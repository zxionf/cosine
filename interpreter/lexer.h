#pragma once
#include "token.h"
using namespace xel::token;

namespace xel::lexer
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
            bool is_letter(char ch);\
            bool next_char_is_letter();
            std::string read_number();
            std::string read_identifier();
            Token new_token(Token::Type type, const std::string& literal);
        private:
            std::string input;
            unsigned int inputlen;
            char ch;
            int pos;
            int next_pos;
    };
}