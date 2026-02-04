#pragma once

#include "expr.h"

namespace xel
{
    class ASTPrinter : public ExprVisitor
    {
        public:
            ~ASTPrinter() = default;

            std::string print(std::shared_ptr<Expr> expr) {
                return std::any_cast<std::string>(expr->accept(this));
            }

            std::any visit_grouping_expr(std::shared_ptr<Grouping> expr) override {
                return "(group " + print(expr->_expression) + ")";
            }

            std::any visit_binary_expr(std::shared_ptr<Binary> expr) override {
                return "(binary " + print(expr->_left) + " " + expr->_op.get_lexeme() + " " + print(expr->_right) + ")";
            }

            std::any visit_literal_expr(std::shared_ptr<Literal> expr) override {
                if (expr->_value.type() == typeid(std::string)) {
                    return "\"" + std::any_cast<std::string>(expr->_value) + "\"";
                }
                return std::to_string(std::any_cast<double>(expr->_value));
            }

            std::any visit_unary_expr(std::shared_ptr<Unary> expr) override {
                return "(unary " + expr->_op.get_lexeme() + print(expr->_right) + ")";
            }
    };
}