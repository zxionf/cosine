#include "evaluator.h"
using namespace xel;
#include "../xel_error.h"

std::any Evaluator::visit_print_stmt(std::shared_ptr<Print> stmt) {
    std::any value = evaluate(stmt->_expression);
    std::printf("%s", stringify(value).c_str());
    return nullptr;
}

std::any Evaluator::visit_expression_stmt(std::shared_ptr<Expression> stmt) {
    evaluate(stmt->_expression);
    return nullptr;
}

std::any Evaluator::visit_var_stmt(std::shared_ptr<Var> stmt) {
    std::any value = nullptr;
    if (stmt->_initializer) {
        value = evaluate(stmt->_initializer);
    }
    _environment.define(stmt->_name.get_lexeme(), value);
    return nullptr;
}

std::any Evaluator::visit_block_stmt(std::shared_ptr<Block> stmt) {
    execute_block(stmt->_statements, Environment(&_environment));
    return nullptr;
}

void Evaluator::execute_block(std::list<std::shared_ptr<Stmt>> statements, Environment environment) {
    Environment previous = _environment;
    try {
        _environment = environment;
        for (auto statement : statements)
            execute(statement);
    } catch (...) {
        environment = previous;
        throw;
    }
    _environment = previous;
}

std::any Evaluator::visit_assign_expr(std::shared_ptr<Assign> expr) {
    std::any value = evaluate(expr->_value);
    _environment.assign(expr->_name, value);
    return value;
}

std::any Evaluator::visit_variable_expr(std::shared_ptr<Variable> expr) {
    return _environment.get(expr->_name);
}

void Evaluator::interpret(const std::list<std::shared_ptr<Stmt>>& statements) {
    try {
        for (auto& stmt : statements) {
            stmt->accept(this);
        }
    } catch (std::runtime_error& error) {
        throw error;
    }
}

void Evaluator::interpret(std::shared_ptr<Expr> expression) {
    try {
        std::any value = evaluate(expression);
        std::printf("%s", stringify(value).c_str());
    } catch (std::runtime_error error) {}
}

std::any Evaluator::visit_literal_expr(std::shared_ptr<Literal> expr) {
    return expr->_value;
}

std::any Evaluator::visit_grouping_expr(std::shared_ptr<Grouping> expr) {
    return evaluate(expr->_expression);
}

std::any Evaluator::evaluate(std::shared_ptr<Expr> expr) {
    return expr->accept(this);
}

std::any Evaluator::visit_unary_expr(std::shared_ptr<Unary> expr) {
    std::any right = evaluate(expr->_right);

    switch (expr->_op.get_type()) {
        case Token::Type::BANG:
            return !is_truthy(right);
        case Token::Type::MINUS:
            check_number_operand(expr->_op, right);
            return - std::any_cast<double>(right);
    }

    // Unreachable.
    return nullptr;
}

bool Evaluator::is_truthy(const std::any& object) {
    if (!object.has_value()) return false;
    if (object.type() == typeid(double)) return std::any_cast<double>(object);
    return true;
}

// TODO : optimize
bool Evaluator::is_equal(const std::any& a, const std::any& b) {
    if (!a.has_value() && !b.has_value()) return true;
    if (!a.has_value() || !b.has_value()) return false;
    if (a.type() != b.type()) return false;

    if(a.type() == typeid(double)) return std::any_cast<double>(a) == std::any_cast<double>(b);
    if (a.type() == typeid(std::string)) return std::any_cast<std::string>(a) == std::any_cast<std::string>(b);

    return false;
  }

std::any Evaluator::visit_binary_expr(std::shared_ptr<Binary> expr) {
    std::any left = evaluate(expr->_left);
    std::any right = evaluate(expr->_right); 

    switch (expr->_op.get_type()) {
        // != ==
        case Token::Type::BANG_EQUAL: return !is_equal(left, right);
        case Token::Type::EQUAL_EQUAL: return is_equal(left, right);
        // > >= < <=
        case Token::Type::GREATER:
            check_number_operands(expr->_op, left, right);
            return std::any_cast<double>(left) > std::any_cast<double>(right);
        case Token::Type::GREATER_EQUAL:
            check_number_operands(expr->_op, left, right);
            return std::any_cast<double>(left) >= std::any_cast<double>(right);
        case Token::Type::LESS:
            check_number_operands(expr->_op, left, right);
            return std::any_cast<double>(left) < std::any_cast<double>(right);
        case Token::Type::LESS_EQUAL:
            check_number_operands(expr->_op, left, right);
            return std::any_cast<double>(left) <= std::any_cast<double>(right);
        // + - * /
        case Token::Type::PLUS:
            if (left.type() == typeid(double) && right.type() == typeid(double))
                return std::any_cast<double>(left) + std::any_cast<double>(right);
            if (left.type() == typeid(std::string) && right.type() == typeid(std::string))
                return std::any_cast<std::string>(left) + std::any_cast<std::string>(right);
            throw error(expr->_op,"Operands must be two numbers or two strings.");
        case Token::Type::MINUS:
            check_number_operands(expr->_op, left, right);
            return std::any_cast<double>(left) - std::any_cast<double>(right);
        case Token::Type::SLASH:
            check_number_operands(expr->_op, left, right);
            return std::any_cast<double>(left) / std::any_cast<double>(right);
        case Token::Type::STAR:
            check_number_operands(expr->_op, left, right);
            return std::any_cast<double>(left) * std::any_cast<double>(right);
    }

    // Unreachable.
    return nullptr;
}

void Evaluator::check_number_operand(const Token& op, const std::any& operand) {
    if (operand.type() == typeid(double)) return;
    throw error(op, "Operand must be a number.");
}

void Evaluator::check_number_operands(const Token& op, const std::any& left, const std::any& right) {
    if (left.type() == typeid(double) && right.type() == typeid(double)) return;
    throw error(op, "Operands must be numbers.");
}

std::runtime_error Evaluator::error(const Token& token, const std::string& message) {
    xel::runtime_error::error(token, message);
    return std::runtime_error(message);
}

std::string Evaluator::stringify(const std::any& object) {
    if (!object.has_value()) return "nil";

    std::string rel;
    if (object.type() == typeid(double)) {
        double num = std::any_cast<double>(object);
        std::string text = std::to_string(num);
        if (text.size() >= 2 && text.substr(text.size() - 2) == ".0") {
            // 移除最后两个字符
            text = text.substr(0, text.size() - 2);
        }
        rel = text;
    }
    else if (object.type() == typeid(std::string)) {
        rel = std::any_cast<std::string>(object);
    }
    else if (object.type() == typeid(bool)){
        rel = std::any_cast<bool>(object) ? "true" : "false";
    }
    return rel;
}