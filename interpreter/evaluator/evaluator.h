#pragma once

#include "../ast/expr.h"

namespace xel
{
    class Evaluator : public ExprVisitor
    {
        public:
            ~Evaluator() = default;

            std::any visit_literal_expr(std::shared_ptr<Literal> expr) override;
            std::any visit_grouping_expr(std::shared_ptr<Grouping> expr) override;
            std::any visit_unary_expr(std::shared_ptr<Unary> expr) override;
            std::any visit_binary_expr(std::shared_ptr<Binary> expr) override;

            std::any evaluate(std::shared_ptr<Expr> expr);
            void interpret(std::shared_ptr<Expr> expression);   

            bool is_truthy(const std::any& object);
            bool is_equal(const std::any& a, const std::any& b);

            void check_number_operand(const Token& op, const std::any& operand);
            void check_number_operands(const Token& op, const std::any& left, const std::any& right);

            std::string stringify(const std::any& object);

            // error
            std::runtime_error error(const Token& token, const std::string& message);
    };
}