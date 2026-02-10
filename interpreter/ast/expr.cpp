#include "expr.h"
using namespace xel;

Binary::Binary(std::shared_ptr<Expr> left, Token op, std::shared_ptr<Expr> right)
    : _left(std::move(left)), _op(std::move(op)), _right(std::move(right)) {}

std::any Binary::accept(ExprVisitor* visitor) {
    return visitor->visit_binary_expr(shared_from_this());
}

Grouping::Grouping(std::shared_ptr<Expr> expression)
    : _expression(std::move(expression)) {}

std::any Grouping::accept(ExprVisitor* visitor) {
    return visitor->visit_grouping_expr(shared_from_this());
}

Literal::Literal(std::any value) : _value(std::move(value)) {}

std::any Literal::accept(ExprVisitor* visitor) {
    return visitor->visit_literal_expr(shared_from_this());
}

Unary::Unary(Token op, std::shared_ptr<Expr> right)
    : _op(std::move(op)), _right(std::move(right)) {}

std::any Unary::accept(ExprVisitor* visitor) {
    return visitor->visit_unary_expr(shared_from_this());
}

Variable::Variable(Token name)
    : _name(std::move(name)) {}

std::any Variable::accept(ExprVisitor* visitor) {
    return visitor->visit_variable_expr(shared_from_this());
}

Assign::Assign(Token name, std::shared_ptr<Expr> value)
    : _name(std::move(name)), _value(std::move(value)) {}

std::any Assign::accept(ExprVisitor* visitor) {
    return visitor->visit_assign_expr(shared_from_this());
}

Logical::Logical(std::shared_ptr<Expr> left, Token op, std::shared_ptr<Expr> right)
    : _left(std::move(left)), _op(std::move(op)), _right(std::move(right)) {}

std::any Logical::accept(ExprVisitor* visitor) {
    return visitor->visit_logical_expr(shared_from_this());
}

Call::Call(std::shared_ptr<Expr> callee, Token paren, std::vector<std::shared_ptr<Expr>> arguments)
    : _callee(std::move(callee)), _paren(std::move(paren)), _arguments(std::move(arguments)) {}

std::any Call::accept(ExprVisitor* visitor) {
    return visitor->visit_call_expr(shared_from_this());
}
