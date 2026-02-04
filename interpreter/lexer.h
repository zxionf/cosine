#pragma once
#include "token.h"

#include <list>

namespace xel
{
    class Lexer
    {
        public:
            Lexer() = default;
            Lexer(const std::string& file);
            Lexer(const char* buffer, size_t size);
            ~Lexer() = default;

            std::list<Token> get_tokens();
        private:
            bool is_at_end();
            void get_token();
            char next_char();
            bool next_char_is(char ch);
            char peek_char();
            char peek_next_char();

            void read_string();
            void read_number();
            void read_identifier();
            bool is_digit(char ch);
            bool is_alpha(char ch); // 字母或者下划线
            bool is_alpha_numeric(char ch); // 数字或者字母或者下划线

            void add_token(Token::Type type);
            void add_token(Token::Type type, const std::any& literal);

            void skip_whitespace();
            void read_char();
            void unread_char();
            
            
            bool next_char_is_letter();
            // std::string read_number();
            // std::string read_identifier();
            Token new_token(Token::Type type, const std::string& literal);
        private:
            std::string source;
            std::list<Token> tokens;

            int start = 0;
            int current = 0;
            int line = 1;

            static std::map<std::string, Token::Type> keywords;
    };
}