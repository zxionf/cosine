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
