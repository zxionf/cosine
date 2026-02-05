#pragma once

#include "expr.h"

namespace xel
{

    class Stmt;
    class Expression;
    class Print;
    class Var;

    class StmtVisitor
    {
        public:
            virtual ~StmtVisitor() = default;

            virtual std::any visit_expression_stmt(std::shared_ptr<Expression> stmt) = 0;
            virtual std::any visit_print_stmt(std::shared_ptr<Print> stmt) = 0;
            virtual std::any visit_var_stmt(std::shared_ptr<Var> stmt) = 0;
    };

    class Stmt
    {
        public:
            virtual ~Stmt() = default;
            virtual std::any accept(StmtVisitor* visitor) = 0;
    };

    class Expression : public Stmt, public std::enable_shared_from_this<Expression>
    {
        public:
            // Expression() = default;
            explicit Expression(std::shared_ptr<Expr> expression) :_expression(std::move(expression)) {}
            std::any accept(StmtVisitor* visitor) override;
        public:
            std::shared_ptr<Expr> _expression;
    };

    class Print : public Stmt, public std::enable_shared_from_this<Print>
    {
        public:
            // Print() = default;
            explicit Print(std::shared_ptr<Expr> expression) :_expression(std::move(expression)) {}
            std::any accept(StmtVisitor* visitor) override;
        public:
            std::shared_ptr<Expr> _expression;
    };

    class Var : public Stmt, public std::enable_shared_from_this<Var>
    {
        public:
            // Var() = default;
            explicit Var(Token name, std::shared_ptr<Expr> initializer) :_name(std::move(name)), _initializer(std::move(initializer)) {}
            std::any accept(StmtVisitor* visitor) override;
        public:
            Token _name;
            std::shared_ptr<Expr> _initializer;
    };
}