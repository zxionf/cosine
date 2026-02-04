#pragma once

#include "../token.h"
using namespace xel;
#include <memory>
#include <vector>
#include <any>

namespace xel
{
    // 前向声明
    class Expr;
    class Binary;
    class Grouping;
    class Literal;
    class Unary;

    // 访问者基类
    class ExprVisitor {
    public:
        virtual ~ExprVisitor() = default;
        
        virtual std::any visitBinaryExpr(std::shared_ptr<Binary> expr) = 0;
        virtual std::any visitGroupingExpr(std::shared_ptr<Grouping> expr) = 0;
        virtual std::any visitLiteralExpr(std::shared_ptr<Literal> expr) = 0;
        virtual std::any visitUnaryExpr(std::shared_ptr<Unary> expr) = 0;
    };

    // 表达式基类
    class Expr {
    public:
        virtual ~Expr() = default;
        virtual std::any accept(ExprVisitor* visitor) = 0;
    };

    // 二元表达式
    class Binary : public Expr, public std::enable_shared_from_this<Binary> {
    public:
        Binary(std::shared_ptr<Expr> left, Token op, std::shared_ptr<Expr> right)
            : _left(std::move(left)), _op(std::move(op)), _right(std::move(right)) {}
        
        std::any accept(ExprVisitor* visitor) override {
            return visitor->visitBinaryExpr(shared_from_this());
        }
        
        std::shared_ptr<Expr> _left;
        Token _op;
        std::shared_ptr<Expr> _right;
    };

    // 分组表达式
    class Grouping : public Expr, public std::enable_shared_from_this<Grouping> {
    public:
        explicit Grouping(std::shared_ptr<Expr> expression)
            : _expression(std::move(expression)) {}
        
        std::any accept(ExprVisitor* visitor) override {
            return visitor->visitGroupingExpr(shared_from_this());
        }
        
        std::shared_ptr<Expr> _expression;
    };

    // 字面值表达式
    class Literal : public Expr, public std::enable_shared_from_this<Literal> {
    public:
        explicit Literal(Object value) : _value(std::move(value)) {}
        
        std::any accept(ExprVisitor* visitor) override {
            return visitor->visitLiteralExpr(shared_from_this());
        }
        
        Object _value;
    };

    // 一元表达式
    class Unary : public Expr, public std::enable_shared_from_this<Unary> {
    public:
        Unary(Token op, std::shared_ptr<Expr> right)
            : _op(std::move(op)), _right(std::move(right)) {}
        
        std::any accept(ExprVisitor* visitor) override {
            return visitor->visitUnaryExpr(shared_from_this());
        }
        
        Token _op;
        std::shared_ptr<Expr> _right;
    };
}