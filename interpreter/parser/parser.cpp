#include "parser.h"
using namespace xel;
#include "../xel_error.h"

std::list<std::shared_ptr<Stmt>> Parser::parse() {
    std::list<std::shared_ptr<Stmt>> statements;
    while (!is_at_end()) {
        statements.emplace_back(declaration());
    }

    return statements;
}

std::shared_ptr<Stmt> Parser::declaration() {
    try {
        if (match(Token::Type::VAR)) return var_declaration();
        return statement();
    } catch (std::runtime_error error) {
        synchronize();
        return nullptr;
    }
  }

  std::shared_ptr<Stmt> Parser::var_declaration() {
    Token& name = consume(Token::Type::IDENTIFIER, "Expect variable name.");

    std::shared_ptr<Expr> initializer = nullptr;
    if (match(Token::Type::EQUAL)) {
        initializer = expression();
    }

    consume(Token::Type::SEMICOLON, "Expect ';' after variable declaration.");
    return std::make_shared<Var>(name, initializer);
  }

std::shared_ptr<Stmt> Parser::statement() {
    if (match(Token::Type::PRINT)) return print_statement();
    if (match(Token::Type::LEFT_BRACE)) return std::make_shared<Block>(block());
    return expression_statement();
}

std::list<std::shared_ptr<Stmt>> Parser::block() {
    std::list<std::shared_ptr<Stmt>> statements{};

    while (!check(Token::Type::RIGHT_BRACE) && !is_at_end()) {
        statements.emplace_back(declaration());
    }

    consume(Token::Type::RIGHT_BRACE, "Expect '}' after block.");
    return statements;
  }

std::shared_ptr<Stmt> Parser::expression_statement() {
    std::shared_ptr<Expr> expr = expression();
    consume(Token::Type::SEMICOLON, "Expect ';' after expression.");
    return std::make_shared<Expression>(expr);
}

std::shared_ptr<Stmt> Parser::print_statement() {
    std::shared_ptr<Expr> value = expression();
    consume(Token::Type::SEMICOLON, "Expect ';' after value.");
    return std::make_shared<Print>(value);
}

std::shared_ptr<Expr> Parser::expression() {
    return assignment();
}

std::shared_ptr<Expr> Parser::assignment() {
    std::shared_ptr<Expr> expr = equality();

    if (match(Token::Type::EQUAL)) {
        Token& equals = prev_token();
        std::shared_ptr<Expr> value = assignment();

        // if (expr instanceof Expr.Variable)
        auto variable = std::dynamic_pointer_cast<Variable>(expr);
        if (variable) {
            Token& name = variable->_name;
            return std::make_shared<Assign>(name, value);
        }

        error(equals, "Invalid assignment target."); 
    }
    return expr;
}

std::shared_ptr<Expr> Parser::equality() {
    std::shared_ptr<Expr> expr = comparison();

    while (match(Token::Type::BANG_EQUAL, Token::Type::EQUAL_EQUAL)) {
        Token& op = prev_token();
        std::shared_ptr<Expr> right = comparison();
        // expr = new Expr.Binary(expr, operator, right);
        expr = std::make_shared<Binary>(expr, op, right);
    }
    return expr;
}

/*
* 假设当前 token 是 PLUS
* match(PLUS, MINUS, STAR);  // 返回 true，token 被消费
* match(NUMBER, STRING);     // 返回 false，token 未被消费
*/
template<typename... Type>
bool Parser::match(Type... types) {
    if ((check(types) || ...)) {
        next_token();
        return true;
    }
    return false;
}

bool Parser::check(Token::Type type) {
    if (is_at_end()) return false;
    return peek_token().get_type() == type;
}

Token& Parser::next_token() {
    if (!is_at_end()) _current++;
    return prev_token();
}

bool Parser::is_at_end() {
    return peek_token().get_type() == Token::Type::TOKEN_EOF;
}

Token& Parser::peek_token() {
    return *_current;
}

Token& Parser::prev_token() {
    return *std::prev(_current);
}

std::shared_ptr<Expr> Parser::comparison() {
    std::shared_ptr<Expr> expr = term();

    while (match(Token::Type::GREATER, Token::Type::GREATER_EQUAL, Token::Type::LESS, Token::Type::LESS_EQUAL)) {
        Token& op = prev_token();
        std::shared_ptr<Expr> right = term();
        // expr = new Expr.Binary(expr, operator, right);
        expr = std::make_shared<Binary>(expr, op, right);
    }
    return expr;
}

std::shared_ptr<Expr> Parser::term() {
    std::shared_ptr<Expr> expr = factor();

    while (match(Token::Type::MINUS, Token::Type::PLUS)) {
        Token& op = prev_token();
        std::shared_ptr<Expr> right = factor();
        // expr = new Expr.Binary(expr, operator, right);
        expr = std::make_shared<Binary>(expr, op, right);
    }
    return expr;
}

std::shared_ptr<Expr> Parser::factor() {
    std::shared_ptr<Expr> expr = unary();

    while (match(Token::Type::SLASH, Token::Type::STAR)) {
        Token& op = prev_token();
        std::shared_ptr<Expr> right = unary();
        // expr = new Expr.Binary(expr, operator, right);
        expr = std::make_shared<Binary>(expr, op, right);
    }
    return expr;
}

std::shared_ptr<Expr> Parser::unary() {
    if (match(Token::Type::BANG, Token::Type::MINUS)) {
        Token& op = prev_token();
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
        return std::make_shared<Literal>(prev_token().get_literal());
    }

    if (match(Token::Type::IDENTIFIER)) {
        // return new Expr.Variable(previous());
        return std::make_shared<Variable>(prev_token());
    }

    if (match(Token::Type::LEFT_PAREN)) {
        std::shared_ptr<Expr> expr = expression();
        consume(Token::Type::RIGHT_PAREN, "Expect ')' after expression.");
        // return new Expr.Grouping(expr);
        return std::make_shared<Grouping>(expr);
    }

    throw error(peek_token(), "Expect expression.");
}

Token& Parser::consume(Token::Type type, const std::string& message) {
    if (check(type)) return next_token();

    throw error(peek_token(), message);
}

std::runtime_error Parser::error(const Token& token, const std::string& message) {
    xel::error::error(token, message);
    return std::runtime_error(message);
}

void Parser::synchronize() {
    next_token();

    while (!is_at_end()) {
        if (prev_token().get_type() == Token::Type::SEMICOLON) return;

        switch (peek_token().get_type()) {
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
        next_token();
    }
}