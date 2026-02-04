#pragma once

#include <string>
#include <map>
#include "object/object.h"
using namespace xel::object;

namespace xel::token
{
    class Token
    {
        public:
            enum Type {
                // 单字符符号
                LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
                COMMA, DOT, MINUS, PLUS, SEMICOLON, SLASH, STAR,

                // 一到两个字符的符号
                BANG, BANG_EQUAL,
                EQUAL, EQUAL_EQUAL,
                GREATER, GREATER_EQUAL,
                LESS, LESS_EQUAL,

                // 文字符号
                IDENTIFIER, STRING, NUMBER,

                // 关键字
                AND, CLASS, ELSE, FALSE, FUN, FOR, IF, NIL, OR,
                PRINT, RETURN, SUPER, THIS, TRUE, VAR, WHILE,

                TOKEN_EOF
            };
            Token() = default;
            Token(Type type, const std::string& lexeme, Object literal, int line) : type(type),lexeme(lexeme),literal(literal),line(line) {}
            ~Token() = default;

            Token& operator=(const Token& other);

            Type get_type() const;
            std::string get_literal() const;
            std::string get_name() const;
            static Type find_keyword(const std::string& ind);

            std::string to_string() const;

        private:
            Type type;              // 类型
            std::string lexeme;     // 词素
            Object literal;         // 值
            int line;               // 行号

            static std::map<Type, std::string> type_map; // 类型->名称
    };
}