#pragma once

#include <list>
#include <memory>

#include "../token.h"
using namespace xel::token;

namespace xel::ast
{
    class Node
    {
        public:
            enum Type
            {
                INTEGER,                // 整数
                FLOAT,                  // 浮点数
                EXPRESSION,             // 表达式
                INFIX,                  // 中缀表达式
                EXPRESSION_STATEMENT,   // 表达式语句
                PROGRAM,                // 程序
            };
            Node() = default;
            Node(Type type) :_type(type) {}
            virtual ~Node() = default;

            Type get_type() const { return _type; }
            std::string get_name();
            void set_token(const Token& token) { _token = token; }
            Token get_token() const { return _token; }

            // debug
            virtual std::string to_string() = 0;

        private:
            Type _type;
            Token _token;
            static std::map<Type, std::string> _type_map;
    };

    class Expression : public Node
    { 
        public:
            Expression() :Node() {}
            Expression(Type type) :Node(type) {}
            ~Expression() = default;
    };

    class Statement : public Node
    { 
        public:
            Statement() :Node() {}
            Statement(Type type) :Node(type) {}
            ~Statement() = default;
    };
}