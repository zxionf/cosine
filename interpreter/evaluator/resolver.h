#pragma once

#include "evaluator.h"
#include <stack>

namespace xel
{
    class Resolver : public ExprVisitor, public StmtVisitor
    {
        private:
            enum FunctionType
            {
                NONE,FUNCTION
            };
        public:
            Resolver(Evaluator* evaluator) :_evaluator(evaluator) {}

            void resolve(const std::list<std::shared_ptr<Stmt>>& statements);

            var visit_block_stmt      (const std::shared_ptr<Block>&      stmt) override;
            var visit_var_stmt        (const std::shared_ptr<Var>&        stmt) override;
            var visit_function_stmt   (const std::shared_ptr<Function>&   stmt) override;
            var visit_expression_stmt (const std::shared_ptr<Expression>& stmt) override;
            var visit_if_stmt         (const std::shared_ptr<If>&         stmt) override;
            var visit_print_stmt      (const std::shared_ptr<Print>&      stmt) override;
            var visit_return_stmt     (const std::shared_ptr<Return>&     stmt) override;
            var visit_while_stmt      (const std::shared_ptr<While>&      stmt) override;

            var visit_variable_expr(const std::shared_ptr<Variable>& expr) override;
            var visit_assign_expr  (const std::shared_ptr<Assign>&   expr) override;
            var visit_binary_expr  (const std::shared_ptr<Binary>&   expr) override;
            var visit_call_expr    (const std::shared_ptr<Call>&     expr) override;
            var visit_grouping_expr(const std::shared_ptr<Grouping>& expr) override;
            var visit_literal_expr (const std::shared_ptr<Literal>&  expr) override;
            var visit_logical_expr (const std::shared_ptr<Logical>&  expr) override;
            var visit_unary_expr   (const std::shared_ptr<Unary>&    expr) override;
        private:
            void resolve(const std::shared_ptr<Stmt>& stmt);
            void resolve(const std::shared_ptr<Expr>& expr);
            void begin_scope();
            void end_scope();
            void declare(const Token& name);
            void define(const Token& name);
            void resolve_local(const std::shared_ptr<Expr>& expr, const Token& name);
            void resolve_function(const std::shared_ptr<Function>& function, FunctionType type);
        private:
            Evaluator* _evaluator;
            // Stack<Map<String, Boolean>> scopes = new Stack<>();
            std::vector<std::unordered_map<std::string, bool>> _scopes{};
            FunctionType _current_function = NONE;
    };
}