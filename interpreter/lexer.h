#pragma once

#include "token.h"
#include <list>

namespace xel
{
    class Lexer
    {
        public:
            Lexer() = default;
            Lexer(const std::string& file);             // 从文件读取
            Lexer(const char* buffer, size_t size);     // 从内存中读取
            ~Lexer() = default;

            std::list<Token> get_tokens();      // 获取所有token
        private:
            bool is_at_end();
            void get_token();
            char next_char();           // _current++
            bool next_char_is(char ch);
            char peek_char();
            char peek_next_char();      // peek 后面第一个字符

            void read_string();
            void read_number();
            void read_identifier();
            bool is_digit(char ch);
            bool is_alpha(char ch); // 字母或者下划线
            bool is_alpha_numeric(char ch); // 数字或者字母或者下划线

            void add_token(Token::Type type);
            void add_token(Token::Type type, const var& literal);
            
        private:
            std::string source;
            std::list<Token> tokens;

            int start = 0;
            int current = 0;
            int line = 1;

            static std::map<std::string, Token::Type> keywords;
    };
}