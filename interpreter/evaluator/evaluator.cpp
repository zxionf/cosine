#include "evaluator.h"
using namespace xel;
#include "../xel_error.h"
#include "../ast/callable.h"
#include <type_traits>
#include <cmath>

Evaluator::Evaluator() {
    _globals = new Environment();
    _environment = _globals;

    _globals->define("clock", std::make_shared<clock>());
}

var Evaluator::visit_print_stmt(const std::shared_ptr<Print>& stmt) {
    var value = evaluate(stmt->_expression);
    std::printf("%s", stringify(value).c_str());
    return nullptr;
}

var Evaluator::visit_expression_stmt(const std::shared_ptr<Expression>& stmt) {
    evaluate(stmt->_expression);
    return nullptr;
}

var Evaluator::visit_if_stmt(const std::shared_ptr<If>& stmt) {
    if (is_truthy(evaluate(stmt->_condition))) {
        execute(stmt->_then_branch);
    } else if (stmt->_else_branch) {
        execute(stmt->_else_branch);
    }
    return nullptr;
}

var Evaluator::visit_while_stmt(const std::shared_ptr<While>& stmt) {
    while (is_truthy(evaluate(stmt->_condition))) {
        execute(stmt->_body);
    }
    return nullptr;
}

var Evaluator::visit_var_stmt(const std::shared_ptr<Var>& stmt) {
    var value = nullptr;
    if (stmt->_initializer) {
        value = evaluate(stmt->_initializer);
    }
    _environment->define(stmt->_name.get_lexeme(), value);
    return nullptr;
}

var Evaluator::visit_return_stmt(const std::shared_ptr<Return>& stmt) {
    var value = nullptr;
    if (stmt->_value != nullptr)
        value = evaluate(stmt->_value);
    throw xel::error::Return(value);
}

var Evaluator::visit_function_stmt(const std::shared_ptr<Function>& stmt) {
    std::shared_ptr<function> func = std::make_shared<function>(stmt, _environment);
    _environment->define(stmt->_name.get_lexeme(), func);
    return nullptr;
}

var Evaluator::visit_block_stmt(const std::shared_ptr<Block>& stmt) {
    execute_block(stmt->_statements, new Environment(_environment));
    return nullptr;
}

void Evaluator::execute_block(const std::list<std::shared_ptr<Stmt>>& statements, Environment* environment) {
    Environment* previous = _environment;
    try {
        _environment = environment;
        for (auto statement : statements)
            execute(statement);
    } catch (...) {
        _environment = previous;
        throw;
    }
    _environment = previous;
}

var Evaluator::visit_call_expr(const std::shared_ptr<Call>& expr) {
    var callee = evaluate(expr->_callee);

    std::vector<var> arguments;
    for (auto argument : expr->_arguments)
        arguments.push_back(evaluate(argument));

    auto* func = std::get_if<std::shared_ptr<Callable>>(&callee);
    if(!func) throw error(expr->_paren, "Can only call functions and classes.");

    if (arguments.size() != (*func)->arity())
        throw error(expr->_paren, "Expected " + std::to_string((*func)->arity()) + "arguments but got " + std::to_string(arguments.size()) + " .");
    return (*func)->call(this, arguments);
}

var Evaluator::visit_logical_expr(const std::shared_ptr<Logical>& expr) {
    var left = evaluate(expr->_left);

    if (expr->_op.get_type() == Token::Type::OR)
        if (is_truthy(left)) return left;
    if (expr->_op.get_type() == Token::Type::AND)
        if (!is_truthy(left)) return left;

    return evaluate(expr->_right);
}

var Evaluator::visit_assign_expr(const std::shared_ptr<Assign>& expr) {
    var value = evaluate(expr->_value);
    _environment->assign(expr->_name, value);
    return value;
}

var Evaluator::visit_variable_expr(const std::shared_ptr<Variable>& expr) {
    return _environment->get(expr->_name);
}

void Evaluator::interpret(const std::list<std::shared_ptr<Stmt>>& statements) {
    try {
        for (auto& stmt : statements) {
            stmt->accept(this);
        }
    } catch (xel::runtime_error& error) {
        std::printf("%s\n", error.message.c_str());
        throw;
    }
}

void Evaluator::interpret(const std::shared_ptr<Expr>& expression) {
    try {
        var value = evaluate(expression);
        std::printf("%s", stringify(value).c_str());
    } catch (std::runtime_error error) {}
}

var Evaluator::visit_literal_expr(const std::shared_ptr<Literal>& expr) {
    return expr->_value;
}

var Evaluator::visit_grouping_expr(const std::shared_ptr<Grouping>& expr) {
    return evaluate(expr->_expression);
}

var Evaluator::evaluate(const std::shared_ptr<Expr>& expr) {
    return expr->accept(this);
}

var Evaluator::visit_unary_expr(const std::shared_ptr<Unary>& expr) {
    var right = evaluate(expr->_right);

    switch (expr->_op.get_type()) {
        case Token::Type::BANG:
            return !is_truthy(right);
        case Token::Type::MINUS:
            check_number_operand(expr->_op, right);
            // return - std::any_cast<double>(right);
            return - *std::get_if<double>(&right);
    }

    // Unreachable.
    return nullptr;
}

// TODO : more types
bool Evaluator::is_truthy(const var& object) {
    return std::visit([](auto&& arg) -> bool {
        using T = std::decay_t<decltype(arg)>;
        
        if (std::is_same_v<T, std::nullptr_t>) return false;
        if constexpr (std::is_same_v<T, bool>) return arg;
        if constexpr (std::is_same_v<T, double>) return arg != 0.0 && !std::isnan(arg);
        if constexpr (std::is_same_v<T, std::string>) return !arg.empty();
        // 不会有其他类型
        return true;
    },object);
}

// TODO : optimize, more checks
bool Evaluator::is_equal(const var& a, const var& b) {
    if (a.index() != b.index()) return false;
    return std::visit(overloaded{
        [](std::nullptr_t, std::nullptr_t) { return true; },
        [](bool lhs, bool rhs) { return lhs == rhs; },
        [](double lhs, double rhs) {
            constexpr double eps = 1e-10;
            return std::abs(lhs - rhs) < eps;
        },
        [](const std::string& lhs, const std::string& rhs) {
            return lhs == rhs;
        },
        [](auto&&, auto&&) { return false; }  // 处理其他类型（如果有）
    }, a, b);
}

var Evaluator::visit_binary_expr(const std::shared_ptr<Binary>& expr) {
    var left = evaluate(expr->_left);
    var right = evaluate(expr->_right); 

    switch (expr->_op.get_type()) {
        // != ==
        case Token::Type::BANG_EQUAL: return !is_equal(left, right);
        case Token::Type::EQUAL_EQUAL: return is_equal(left, right);
        // > >= < <=
        case Token::Type::GREATER:
            check_number_operands(expr->_op, left, right);
            return std::get<double>(left) > std::get<double>(right);
        case Token::Type::GREATER_EQUAL:
            check_number_operands(expr->_op, left, right);
            return std::get<double>(left) >= std::get<double>(right);
        case Token::Type::LESS:
            check_number_operands(expr->_op, left, right);
            return std::get<double>(left) < std::get<double>(right);
        case Token::Type::LESS_EQUAL:
            check_number_operands(expr->_op, left, right);
            return std::get<double>(left) <= std::get<double>(right);
        // + - * /
        case Token::Type::PLUS:
            // number + number
            if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right))
                return std::get<double>(left) + std::get<double>(right);
            // string + string
            if (std::holds_alternative<std::string>(left) && std::holds_alternative<std::string>(right))
                return std::get<std::string>(left) + std::get<std::string>(right);
            // string + number
            if (std::holds_alternative<std::string>(left) || std::holds_alternative<double>(right))
                return std::get<std::string>(left) + std::to_string(std::get<double>(right));
            // number + string
            if (std::holds_alternative<double>(left) && std::holds_alternative<std::string>(right))
                return std::to_string(std::get<double>(left)) + std::get<std::string>(right);
            throw error(expr->_op,"Operands must be two numbers or two strings.");
        case Token::Type::MINUS:
            check_number_operands(expr->_op, left, right);
            return std::get<double>(left) - std::get<double>(right);
        case Token::Type::SLASH:
            check_number_operands(expr->_op, left, right);
            return std::get<double>(left) / std::get<double>(right);
        case Token::Type::STAR:
            check_number_operands(expr->_op, left, right);
            return std::get<double>(left) * std::get<double>(right);
    }

    // Unreachable.
    return nullptr;
}

void Evaluator::check_number_operand(const Token& op, const var& operand) {
    if (std::holds_alternative<double>(operand)) return;
    throw error(op, "Operand must be a number.");
}

void Evaluator::check_number_operands(const Token& op, const var& left, const var& right) {
    if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) return;
    throw error(op, "Operands must be numbers.");
}

xel::runtime_error Evaluator::error(const Token& token, const std::string& message) {
    return xel::error::error_(token, message);
}

std::string Evaluator::stringify(const var& object) {
    if (std::holds_alternative<std::nullptr_t>(object)) return "nil";

    std::string rel;
    if (std::holds_alternative<double>(object)) {
        double num = std::get<double>(object);
        std::string text = std::to_string(num);
        if (text.size() >= 2 && text.substr(text.size() - 2) == ".0") {
            // 移除最后两个字符
            text = text.substr(0, text.size() - 2);
        }
        rel = text;
    }
    if (std::holds_alternative<std::string>(object))
        rel = std::get<std::string>(object);
    if (std::holds_alternative<bool>(object))
        rel = std::get<bool>(object) ? "true" : "false";
    return rel;
}