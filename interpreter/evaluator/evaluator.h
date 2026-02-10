#pragma once

#include "../ast/stmt.h"
#include "../xel_error.h"
#include "../environment/environment.h"

namespace xel
{
    class Evaluator : public ExprVisitor, public StmtVisitor
    {
        public:
            Evaluator();
            ~Evaluator() = default;

            void interpret(const std::shared_ptr<Expr>& expression);
            void interpret(const std::list<std::shared_ptr<Stmt>>& statements);
            void execute_block(const std::list<std::shared_ptr<Stmt>>& statements, Environment* environment);

        private:
            var visit_literal_expr    (const std::shared_ptr<Literal>&    expr) override;
            var visit_grouping_expr   (const std::shared_ptr<Grouping>&   expr) override;
            var visit_unary_expr      (const std::shared_ptr<Unary>&      expr) override;
            var visit_binary_expr     (const std::shared_ptr<Binary>&     expr) override;
            var visit_variable_expr   (const std::shared_ptr<Variable>&   expr) override;
            var visit_assign_expr     (const std::shared_ptr<Assign>&     expr) override;
            var visit_logical_expr    (const std::shared_ptr<Logical>&    expr) override;
            var visit_call_expr       (const std::shared_ptr<Call>&       expr) override;

            var visit_print_stmt      (const std::shared_ptr<Print>&      stmt) override;
            var visit_expression_stmt (const std::shared_ptr<Expression>& stmt) override;
            var visit_var_stmt        (const std::shared_ptr<Var>&        stmt) override;
            var visit_block_stmt      (const std::shared_ptr<Block>&      stmt) override;
            var visit_if_stmt         (const std::shared_ptr<If>&         stmt) override;
            var visit_while_stmt      (const std::shared_ptr<While>&      stmt) override;
            var visit_function_stmt   (const std::shared_ptr<Function>&   stmt) override;
            var visit_return_stmt     (const std::shared_ptr<Return>&     stmt) override;

            var evaluate(const std::shared_ptr<Expr>& expr);
            void execute(const std::shared_ptr<Stmt>& stmt) { stmt->accept(this); }

            bool is_truthy(const var& object);
            bool is_equal(const var& a, const var& b);

            void check_number_operand(const Token& op, const var& operand);
            void check_number_operands(const Token& op, const var& left, const var& right);

            std::string stringify(const var& object);

            // error
            xel::runtime_error error(const Token& token, const std::string& message);
        public:
            Environment* _globals ; // = {new Environment};
            Environment* _environment ; // = _globals;
    };
}