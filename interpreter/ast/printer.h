#pragma once

#include "expr.h"

namespace xel
{
    class ASTPrinter : public ExprVisitor
    {
        public:
            std::string print(std::shared_ptr<Expr> expr) {
                return std::any_cast<std::string>(expr->accept(this));
            }

            std::any visitGroupingExpr(std::shared_ptr<Grouping> expr) override {
                return "(" + print(expr->_expression) + ")";
            }

            std::any visitBinaryExpr(std::shared_ptr<Binary> expr) override {
                return "(" + print(expr->_left) + " " + expr->_op.get_lexeme() + " " + print(expr->_right) + ")";
            }

            std::any visitLiteralExpr(std::shared_ptr<Literal> expr) override {
                if (expr->_value._type == Object::Type::STRING) {
                    return "\"" + std::any_cast<std::string>(expr->_value._value) + "\"";
                }
                return std::to_string(std::any_cast<double>(expr->_value));
            }

            std::any visitUnaryExpr(std::shared_ptr<Unary> expr) override {
                return "(" + expr->_op.get_lexeme() + print(expr->_right) + ")";
            }

            ~ASTPrinter() = default;

    };
}