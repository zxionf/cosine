#pragma once

#include "../token.h"

#include <memory>
#include <vector>

namespace xel
{
    // 前向声明
    class Expr;
    class Binary;
    class Grouping;
    class Literal;
    class Unary;
    class Variable;

    // 访问者基类
    class ExprVisitor 
    {
        public:
            virtual ~ExprVisitor() = default;
            
            virtual std::any visit_binary_expr  (std::shared_ptr<Binary>    expr) = 0;
            virtual std::any visit_grouping_expr(std::shared_ptr<Grouping>  expr) = 0;
            virtual std::any visit_literal_expr (std::shared_ptr<Literal>   expr) = 0;
            virtual std::any visit_unary_expr   (std::shared_ptr<Unary>     expr) = 0;
            virtual std::any visit_variable_expr(std::shared_ptr<Variable>  expr) = 0;
    };

    // 表达式基类
    class Expr 
    {
        public:
            virtual ~Expr() = default;
            virtual std::any accept(ExprVisitor* visitor) = 0;
    };

    // 二元表达式
    class Binary : public Expr, public std::enable_shared_from_this<Binary> 
    {
        public:
            explicit Binary(std::shared_ptr<Expr> left, Token op, std::shared_ptr<Expr> right);
            
            std::any accept(ExprVisitor* visitor) override;
            
            std::shared_ptr<Expr> _left;
            Token _op;
            std::shared_ptr<Expr> _right;
    };

    // 分组表达式
    class Grouping : public Expr, public std::enable_shared_from_this<Grouping>
    {
        public:
            explicit Grouping(std::shared_ptr<Expr> expression);
            
            std::any accept(ExprVisitor* visitor) override;
            
            std::shared_ptr<Expr> _expression;
    };

    // 字面值表达式
    class Literal : public Expr, public std::enable_shared_from_this<Literal>
    {
        public:
            explicit Literal(std::any value); // 缺点：对于左值有一次拷贝
            
            std::any accept(ExprVisitor* visitor) override;
            
            std::any _value;
    };

    // 一元表达式
    class Unary : public Expr, public std::enable_shared_from_this<Unary>
    {
        public:
            explicit Unary(Token op, std::shared_ptr<Expr> right);
            
            std::any accept(ExprVisitor* visitor) override;
            
            Token _op;
            std::shared_ptr<Expr> _right;
    };

    class Variable : public Expr, public std::enable_shared_from_this<Variable>
    {
        public:
            explicit Variable(Token name);
            
            std::any accept(ExprVisitor* visitor) override;
            
            Token _name;
    };
}