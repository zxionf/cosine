#pragma once

#include "../token.h"

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
    class Assign;
    class Logical;
    class Call;

    // 访问者基类
    class ExprVisitor 
    {
        public:
            virtual ~ExprVisitor() = default;
            
            virtual var visit_binary_expr  (const std::shared_ptr<Binary>&    expr) = 0;
            virtual var visit_grouping_expr(const std::shared_ptr<Grouping>&  expr) = 0;
            virtual var visit_literal_expr (const std::shared_ptr<Literal>&   expr) = 0;
            virtual var visit_unary_expr   (const std::shared_ptr<Unary>&     expr) = 0;
            virtual var visit_variable_expr(const std::shared_ptr<Variable>&  expr) = 0;
            virtual var visit_assign_expr  (const std::shared_ptr<Assign>&    expr) = 0;
            virtual var visit_logical_expr (const std::shared_ptr<Logical>&   expr) = 0;
            virtual var visit_call_expr    (const std::shared_ptr<Call>&      expr) = 0;
    };

    // 表达式基类
    class Expr 
    {
        public:
            virtual ~Expr() = default;
            virtual var accept(ExprVisitor* visitor) = 0;
    };

    // 赋值
    class Assign : public Expr, public std::enable_shared_from_this<Assign>
    {
        public:
            explicit Assign(Token name, std::shared_ptr<Expr> value);
            
            var accept(ExprVisitor* visitor) override;
            
            Token _name;
            std::shared_ptr<Expr> _value;
    };

    // 二元表达式
    class Binary : public Expr, public std::enable_shared_from_this<Binary> 
    {
        public:
            explicit Binary(std::shared_ptr<Expr> left, Token op, std::shared_ptr<Expr> right);
            
            var accept(ExprVisitor* visitor) override;
            
            std::shared_ptr<Expr> _left;
            Token _op;
            std::shared_ptr<Expr> _right;
    };

    // 分组表达式
    class Grouping : public Expr, public std::enable_shared_from_this<Grouping>
    {
        public:
            explicit Grouping(std::shared_ptr<Expr> expression);
            
            var accept(ExprVisitor* visitor) override;
            
            std::shared_ptr<Expr> _expression;
    };

    // 字面值表达式
    class Literal : public Expr, public std::enable_shared_from_this<Literal>
    {
        public:
            explicit Literal(var value); // 缺点：对于左值有一次拷贝
            
            var accept(ExprVisitor* visitor) override;
            
            var _value;
    };

    // 一元表达式
    class Unary : public Expr, public std::enable_shared_from_this<Unary>
    {
        public:
            explicit Unary(Token op, std::shared_ptr<Expr> right);
            
            var accept(ExprVisitor* visitor) override;
            
            Token _op;
            std::shared_ptr<Expr> _right;
    };

    // 变量表达式
    class Variable : public Expr, public std::enable_shared_from_this<Variable>
    {
        public:
            explicit Variable(Token name);
            
            var accept(ExprVisitor* visitor) override;
            
            Token _name;
    };

    // 逻辑表达式
    class Logical : public Expr, public std::enable_shared_from_this<Logical>
    {
        public:
            explicit Logical(std::shared_ptr<Expr> left, Token op, std::shared_ptr<Expr> right);
            
            var accept(ExprVisitor* visitor) override;
            
            std::shared_ptr<Expr> _left;
            Token _op;
            std::shared_ptr<Expr> _right;
    };

    // 函数
    class Call : public Expr, public std::enable_shared_from_this<Call>
    {
        public:
            explicit Call(std::shared_ptr<Expr> callee, Token paren, std::vector<std::shared_ptr<Expr>> arguments);
            
            var accept(ExprVisitor* visitor) override;
            
            std::shared_ptr<Expr> _callee; // 被调者
            Token _paren; // 右括号 : 用于运行时错误
            std::vector<std::shared_ptr<Expr>> _arguments; // 参数
    };
}