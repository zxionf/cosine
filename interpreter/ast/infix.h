#pragma once

#include "node.h"

namespace xel::ast
{
    class Infix : public Expression
    {
    public:
        Infix() :Expression(Type::INFIX) {}
        ~Infix() = default;
        // Infix(Expression* left, Token op, Expression* right) : Expression(Type::INFIX), left(left), op(op), right(right) {}

        virtual std::string to_string() override
        {
            return "{(}" + _left->to_string() + "{" + _operator + "}" + _right->to_string() + "{)}";
        }

    public:
        std::string _operator;
        std::shared_ptr<Expression> _left;
        std::shared_ptr<Expression> _right;
    };
}