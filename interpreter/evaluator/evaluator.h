#pragma once

#include "../ast/expr.h"

#include <any>
#include <memory>

namespace xel
{
    class Evaluator : public ExprVisitor
    {
        public:
            ~Evaluator() = default;
            std::any visitLiteralExpr(std::shared_ptr<Literal> expr) override;
            std::any visitGroupingExpr(std::shared_ptr<Grouping> expr) override;

            std::any evaluate(std::shared_ptr<Expr> expr);

            void interpret(std::shared_ptr<Expr> expression);

            // 计算一元表达式
            std::any visitUnaryExpr(std::shared_ptr<Unary> expr) override;
            // 计算二元表达式
            std::any visitBinaryExpr(std::shared_ptr<Binary> expr) override;

            bool is_truthy(const std::any& object);
            bool is_equal(const std::any& a, const std::any& b);

            void check_number_operand(const Token& op, const std::any& operand);
            void check_number_operands(const Token& op, const std::any& left, const std::any& right);

            std::string stringify(const std::any& object);

            // error
            std::runtime_error error(const Token& token, const std::string& message);
    };
}