#pragma once

#include "stmt.h"

namespace xel
{
    class ASTPrinter : public ExprVisitor, public StmtVisitor
    {
        public:
            ~ASTPrinter() = default;

            // Expr
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
                // TODO : more types
                return std::to_string(std::any_cast<double>(expr->_value));
            }

            std::any visit_unary_expr(std::shared_ptr<Unary> expr) override {
                return "(unary " + expr->_op.get_lexeme() + print(expr->_right) + ")";
            }

            std::any visit_variable_expr(std::shared_ptr<Variable> expr) override {
                return "(variable " + expr->_name.get_lexeme() + ")";
            }

            std::any visit_assign_expr(std::shared_ptr<Assign> expr) override {
                return "(assign " + expr->_name.get_lexeme() + " " + print(expr->_value) + ")";
            }

            std::any visit_logical_expr(std::shared_ptr<Logical> expr) override {
                return "(logical " + print(expr->_left) + " " + expr->_op.get_lexeme() + " " + print(expr->_right) + ")";
            }

            // Stmt

            std::string print(std::shared_ptr<Stmt> stmt){
                return std::any_cast<std::string>(stmt->accept(this));
            }

            std::any visit_block_stmt(std::shared_ptr<Block> stmt) override {
                std::string result = "(block";
                for (auto& stmt : stmt->_statements) {
                    result += " " + print(stmt);
                }
                return result + ")";
            }

            std::any visit_expression_stmt(std::shared_ptr<Expression> stmt) override {
                return "(expression " + print(stmt->_expression) + ")";
            }

            std::any visit_print_stmt(std::shared_ptr<Print> stmt) override {
                return "(print " + print(stmt->_expression) + ")";
            }

            std::any visit_var_stmt(std::shared_ptr<Var> stmt) override {
                if (stmt->_initializer) {
                    return "(var " + stmt->_name.get_lexeme() + " " + print(stmt->_initializer) + ")";
                }
                return "(var " + stmt->_name.get_lexeme() + ")";
            }

            std::any visit_if_stmt(std::shared_ptr<If> stmt) override {
                if (stmt->_else_branch) {
                    return "(if " + print(stmt->_condition) + " " + print(stmt->_then_branch) + " " + print(stmt->_else_branch) + ")";
                }
                return "(if " + print(stmt->_condition) + " " + print(stmt->_then_branch) + ")";
            }

            std::any visit_while_stmt(std::shared_ptr<While> stmt) override {
                return "(while " + print(stmt->_condition) + " " + print(stmt->_body) + ")";
            }

            
    };
}