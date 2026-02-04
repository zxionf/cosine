#include "parser.h"
#include "runtime_error.h"

std::shared_ptr<Expr> Parser::parse() {
    try {
        return expression();
    } catch (std::runtime_error error) {
        return nullptr;
    }
}

std::shared_ptr<Expr> Parser::expression() {
    return equality();
}

std::shared_ptr<Expr> Parser::equality() {
    std::shared_ptr<Expr> expr = comparison();

    while (match(Token::Type::BANG_EQUAL, Token::Type::EQUAL_EQUAL)) {
        Token op = previous();
        std::shared_ptr<Expr> right = comparison();
        // expr = new Expr.Binary(expr, operator, right);
        expr = std::make_shared<Binary>(expr, op, right);
    }
    return expr;
}

template<typename... Type>
bool Parser::match(Type... types) {
    if ((check(types) || ...)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::check(Token::Type type) {
    if (is_at_end()) return false;
    return peek().get_type() == type;
}

Token Parser::advance() {
    if (!is_at_end()) _current++;
    return previous();
}

bool Parser::is_at_end() {
    return peek().get_type() == Token::Type::TOKEN_EOF;
}

Token Parser::peek() {
    return *_current;
}

Token Parser::previous() {
    return *std::prev(_current);
}

std::shared_ptr<Expr> Parser::comparison() {
    std::shared_ptr<Expr> expr = term();

    while (match(Token::Type::GREATER, Token::Type::GREATER_EQUAL, Token::Type::LESS, Token::Type::LESS_EQUAL)) {
        Token op = previous();
        std::shared_ptr<Expr> right = term();
        // expr = new Expr.Binary(expr, operator, right);
        expr = std::make_shared<Binary>(expr, op, right);
    }
    return expr;
}

std::shared_ptr<Expr> Parser::term() {
    std::shared_ptr<Expr> expr = factor();

    while (match(Token::Type::MINUS, Token::Type::PLUS)) {
        Token op = previous();
        std::shared_ptr<Expr> right = factor();
        // expr = new Expr.Binary(expr, operator, right);
        expr = std::make_shared<Binary>(expr, op, right);
    }
    return expr;
}

std::shared_ptr<Expr> Parser::factor() {
    std::shared_ptr<Expr> expr = unary();

    while (match(Token::Type::SLASH, Token::Type::STAR)) {
        Token op = previous();
        std::shared_ptr<Expr> right = unary();
        // expr = new Expr.Binary(expr, operator, right);
        expr = std::make_shared<Binary>(expr, op, right);
    }
    return expr;
}

std::shared_ptr<Expr> Parser::unary() {
    if (match(Token::Type::BANG, Token::Type::MINUS)) {
        Token op = previous();
        std::shared_ptr<Expr> right = unary();
        // return new Expr.Unary(operator, right);
        return std::make_shared<Unary>(op, right);
    }

    return primary();
}

std::shared_ptr<Expr> Parser::primary() {
    if (match(Token::Type::FALSE)) return std::make_shared<Literal>(false);
    if (match(Token::Type::TRUE)) return std::make_shared<Literal>(true);
    if (match(Token::Type::NIL)) return std::make_shared<Literal>(nullptr);

    if (match(Token::Type::NUMBER, Token::Type::STRING)) {
        // return new Expr.Literal(previous().literal);
        return std::make_shared<Literal>(previous().get_literal());
    }

    if (match(Token::Type::LEFT_PAREN)) {
        std::shared_ptr<Expr> expr = expression();
        consume(Token::Type::RIGHT_PAREN, "Expect ')' after expression.");
        // return new Expr.Grouping(expr);
        return std::make_shared<Grouping>(expr);
    }

    throw error(peek(), "Expect expression.");
}

Token Parser::consume(Token::Type type, const std::string& message) {
    if (check(type)) return advance();

    throw error(peek(), message);
}

std::runtime_error Parser::error(Token token, const std::string& message) {
    xel::runtime_error::error(token, message);
    return std::runtime_error(message);
}

void Parser::synchronize() {
    advance();

    while (!is_at_end()) {
        if (previous().get_type() == Token::Type::SEMICOLON) return;

        switch (peek().get_type()) {
            case Token::Type::CLASS:
            case Token::Type::FUN:
            case Token::Type::VAR:
            case Token::Type::FOR:
            case Token::Type::IF:
            case Token::Type::WHILE:
            case Token::Type::PRINT:
            case Token::Type::RETURN:
                return;
        }
        advance();
    }
}