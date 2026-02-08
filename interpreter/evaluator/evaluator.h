#pragma once

#include "../ast/stmt.h"
#include "../environment/environment.h"
#include <list>

namespace xel
{
    class Evaluator : public ExprVisitor, public StmtVisitor
    {
        public:
            ~Evaluator() = default;

            std::any visit_literal_expr(std::shared_ptr<Literal> expr) override;
            std::any visit_grouping_expr(std::shared_ptr<Grouping> expr) override;
            std::any visit_unary_expr(std::shared_ptr<Unary> expr) override;
            std::any visit_binary_expr(std::shared_ptr<Binary> expr) override;
            std::any visit_variable_expr(std::shared_ptr<Variable> expr) override;
            std::any visit_assign_expr(std::shared_ptr<Assign> expr) override;
            std::any visit_logical_expr(std::shared_ptr<Logical> expr) override;

            std::any visit_print_stmt(std::shared_ptr<Print> stmt) override;
            std::any visit_expression_stmt(std::shared_ptr<Expression> stmt) override;
            std::any visit_var_stmt(std::shared_ptr<Var> stmt) override;
            std::any visit_block_stmt(std::shared_ptr<Block> stmt) override;
            std::any visit_if_stmt(std::shared_ptr<If> stmt) override;
            std::any visit_while_stmt(std::shared_ptr<While> stmt) override;

            std::any evaluate(std::shared_ptr<Expr> expr);
            void interpret(std::shared_ptr<Expr> expression);
            void interpret(const std::list<std::shared_ptr<Stmt>>& statements);
            void execute(std::shared_ptr<Stmt> stmt) { stmt->accept(this); }
            void execute_block(std::list<std::shared_ptr<Stmt>> statements, Environment* environment);

            bool is_truthy(const std::any& object);
            bool is_equal(const std::any& a, const std::any& b);

            void check_number_operand(const Token& op, const std::any& operand);
            void check_number_operands(const Token& op, const std::any& left, const std::any& right);

            std::string stringify(const std::any& object);

            // error
            std::runtime_error error(const Token& token, const std::string& message);
        public:
            Environment* _environment{new Environment(nullptr)};
    };
}