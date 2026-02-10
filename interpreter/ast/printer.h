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
                // return var_cast<std::string>(expr->accept(this));
                return std::get<std::string>(expr->accept(this));
            }

            var visit_grouping_expr(const std::shared_ptr<Grouping>& expr) override {
                return "(group " + print(expr->_expression) + ")";
            }

            var visit_binary_expr(const std::shared_ptr<Binary>& expr) override {
                return "(binary " + print(expr->_left) + " " + expr->_op.get_lexeme() + " " + print(expr->_right) + ")";
            }

            var visit_literal_expr(const std::shared_ptr<Literal>& expr) override {
                if (std::holds_alternative<std::string>(expr->_value)) {
                    return "\"" + std::get<std::string>(expr->_value) + "\"";
                }
                // if (std::holds_alternative<double>(expr->_value))
                // TODO : more types
                return std::to_string(std::get<double>(expr->_value));
            }

            var visit_unary_expr(const std::shared_ptr<Unary>& expr) override {
                return "(unary " + expr->_op.get_lexeme() + print(expr->_right) + ")";
            }

            var visit_variable_expr(const std::shared_ptr<Variable>& expr) override {
                return "(variable " + expr->_name.get_lexeme() + ")";
            }

            var visit_assign_expr(const std::shared_ptr<Assign>& expr) override {
                return "(assign " + expr->_name.get_lexeme() + " " + print(expr->_value) + ")";
            }

            var visit_logical_expr(const std::shared_ptr<Logical>& expr) override {
                return "(logical " + print(expr->_left) + " " + expr->_op.get_lexeme() + " " + print(expr->_right) + ")";
            }

            var visit_call_expr(const std::shared_ptr<Call>& expr) override {
                std::string result = "(call <" + print(expr->_callee) + ">,";
                for (auto& arg : expr->_arguments) {
                    result += " " + print(arg);
                }
                result += ")";
                return result;
            }

            // Stmt

            std::string print(std::shared_ptr<Stmt> stmt){
                return std::get<std::string>(stmt->accept(this));
            }

            std::string print(std::list<std::shared_ptr<Stmt>> stmts) {
                std::string result = "(block";
                for (auto& stmt : stmts) {
                    result += " " + print(stmt);
                }
                return result + ")";
            }

            var visit_block_stmt(const std::shared_ptr<Block>& stmt) override {
                std::string result = "(block";
                for (auto& stmt : stmt->_statements) {
                    result += " " + print(stmt);
                }
                return result + ")";
            }

            var visit_expression_stmt(const std::shared_ptr<Expression>& stmt) override {
                return "(expression " + print(stmt->_expression) + ")";
            }

            var visit_print_stmt(const std::shared_ptr<Print>& stmt) override {
                return "(print " + print(stmt->_expression) + ")";
            }

            var visit_var_stmt(const std::shared_ptr<Var>& stmt) override {
                if (stmt->_initializer) {
                    return "(var " + stmt->_name.get_lexeme() + " " + print(stmt->_initializer) + ")";
                }
                return "(var " + stmt->_name.get_lexeme() + ")";
            }

            var visit_if_stmt(const std::shared_ptr<If>& stmt) override {
                if (stmt->_else_branch) {
                    return "(if " + print(stmt->_condition) + " " + print(stmt->_then_branch) + " " + print(stmt->_else_branch) + ")";
                }
                return "(if " + print(stmt->_condition) + " " + print(stmt->_then_branch) + ")";
            }

            var visit_while_stmt(const std::shared_ptr<While>& stmt) override {
                return "(while " + print(stmt->_condition) + " " + print(stmt->_body) + ")";
            }

            var visit_function_stmt(const std::shared_ptr<Function>& stmt) override {
                std::string result = "(function " + stmt->_name.get_lexeme() + ")";
                for (auto& param : stmt->_params) {
                    result += " " + param.get_lexeme();
                }
                result += " " + print(stmt->_body);
                return result;
            }

            var visit_return_stmt(const std::shared_ptr<Return>& stmt) override {
                if (stmt->_value) {
                    return "(return " + print(stmt->_value) + ")";
                }
                return "(return)";
            }
    };
}