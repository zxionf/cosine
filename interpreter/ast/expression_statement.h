#pragma once

#include "node.h"

namespace xel::ast
{
    class ExpressionStatement : public Statement
    {
        public:
            ExpressionStatement() : Statement(Type::EXPRESSION_STATEMENT) {}
            ~ExpressionStatement() = default;

            virtual std::string to_string() override
            {
                return _expression->to_string();
            }

        public:
            std::shared_ptr<Expression> _expression;
    };
}