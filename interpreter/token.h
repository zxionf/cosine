#pragma once

#include <string>
#include <map>

namespace xel
{
    namespace token
    {
        class Token
        {
            public:
                enum Type
                {
                    ILLEGAL = 0,    // 非法符号
                    TOKEN_EOF,      // 结束
                    INTEGER,        // 整数
                    FLOAT,          // 浮点数
                    IDENTIFIER,     // 标识符
                    ASSIGN,         // =
                    PLUS,           // +
                    MINUS,          // -
                    ASTERISK,       // *
                    SLASH,          // /
                    MOD,            // %
                    LPAREN,         // (
                    RPAREN,         // )
                    LBRACKET,       // [
                    RBRACKET,       // ]
                    LBRACE,         // {
                    RBRACE,         // }
                    COMMA,          // ,
                    SEMICOLON,      // ;
                    COLON,          // :
                    DOT,            // .
                    QUESTION        // ?
                };
                Token() = default;
                Token(Type type, const std::string& literal) : type(type), literal(literal) {}
                ~Token() = default;

                Token& operator=(const Token& other);

                Type get_type() const;
                std::string get_literal() const;
                std::string get_name() const;

            private:
                Type type;              // 类型
                std::string literal;    // 字面量
                static std::map<Type, std::string> type_map; // 类型名称
        };
    }
}