#include "resolver.h"
using namespace xel;

var Resolver::visit_block_stmt(const std::shared_ptr<Block>& stmt) {
    begin_scope();
    resolve(stmt->_statements);
    end_scope();
    return nullptr;
}

var Resolver::visit_var_stmt(const std::shared_ptr<Var>& stmt) {
    declare(stmt->_name);
    if (stmt->_initializer != nullptr) {
        resolve(stmt->_initializer);
    }
    define(stmt->_name);
    return nullptr;
}

var Resolver::visit_function_stmt(const std::shared_ptr<Function>& stmt) {
    declare(stmt->_name);
    define(stmt->_name);

    resolve_function(stmt, FunctionType::FUNCTION);
    return nullptr;
}

var Resolver::visit_expression_stmt(const std::shared_ptr<Expression>& stmt) {
    resolve(stmt->_expression);
    return nullptr;
}

var Resolver::visit_if_stmt(const std::shared_ptr<If>& stmt) {
    resolve(stmt->_condition);
    resolve(stmt->_then_branch);
    if (stmt->_else_branch != nullptr)
        resolve(stmt->_else_branch);
    return nullptr;
}

var Resolver::visit_print_stmt(const std::shared_ptr<Print>& stmt) {
    resolve(stmt->_expression);
    return nullptr;
}

var Resolver::visit_return_stmt(const std::shared_ptr<Return>& stmt) {
    if (_current_function == FunctionType::NONE)
        xel::error::_error(stmt->_keyword, "Can't return from top-level code.");
    if (stmt->_value != nullptr)
      resolve(stmt->_value);
    return nullptr;
}

var Resolver::visit_while_stmt(const std::shared_ptr<While>& stmt) {
    resolve(stmt->_condition);
    resolve(stmt->_body);
    return nullptr;
}

var Resolver::visit_variable_expr(const std::shared_ptr<Variable>& expr) {
    if (!_scopes.empty() &&
        _scopes.back()[expr->_name.get_lexeme()] == false) {
        xel::error::_error(expr->_name, "Can't read local variable in its own initializer.");
    }

    resolve_local(expr, expr->_name);
    return nullptr;
}

var Resolver::visit_assign_expr(const std::shared_ptr<Assign>& expr) {
    resolve(expr->_value);
    resolve_local(expr, expr->_name);
    return nullptr;
}

var Resolver::visit_binary_expr(const std::shared_ptr<Binary>& expr) {
    resolve(expr->_left);
    resolve(expr->_right);
    return nullptr;
}

var Resolver::visit_call_expr(const std::shared_ptr<Call>& expr) {
    resolve(expr->_callee);
    for (auto& arg : expr->_arguments)
        resolve(arg);
    return nullptr;
}

var Resolver::visit_grouping_expr(const std::shared_ptr<Grouping>& expr) {
    resolve(expr->_expression);
    return nullptr;
}

var Resolver::visit_literal_expr(const std::shared_ptr<Literal>& expr) {
    return nullptr;
}

var Resolver::visit_logical_expr(const std::shared_ptr<Logical>& expr) {
    resolve(expr->_left);
    resolve(expr->_right);
    return nullptr;
}

var Resolver::visit_unary_expr(const std::shared_ptr<Unary>& expr) {
    resolve(expr->_right);
    return nullptr;
}

void Resolver::resolve(const std::list<std::shared_ptr<Stmt>>& statements) {
    for (auto& stmt : statements) {
        resolve(stmt);
    }
}

void Resolver::resolve(const std::shared_ptr<Stmt>& stmt) {
    stmt->accept(this);
}

void Resolver::resolve(const std::shared_ptr<Expr>& expr) {
    expr->accept(this);
}

void Resolver::begin_scope() {
    _scopes.emplace_back(std::unordered_map<std::string, bool>{});
}

void Resolver::end_scope() {
    _scopes.pop_back();
}

void Resolver::declare(const Token& name) {
    if(_scopes.empty()) return;
    auto &scope = _scopes.back();
    if (scope.contains(name.get_lexeme()))
        xel::error::_error(name, "Already a variable with this name in this scope.");
    scope.emplace(name.get_lexeme(), false);
}

void Resolver::define(const Token& name) {
    if (_scopes.empty()) return;
    _scopes.back().emplace(name.get_lexeme(), true);
}

void Resolver::resolve_local(const std::shared_ptr<Expr>& expr, const Token& name) {
    for (int i = static_cast<int>(_scopes.size()) - 1; i >= 0; i--) {
        if (_scopes.at(i).find(name.get_lexeme()) != _scopes.at(i).end()) {
            _evaluator->execute(expr, static_cast<int>(_scopes.size()) - 1 - i);
            return;
        }
    }
}

void Resolver::resolve_function(const std::shared_ptr<Function>& function, FunctionType type) {
    FunctionType enclosing_function = _current_function;
    _current_function = type;
    begin_scope();
    for (Token& param : function->_params) {
        declare(param);
        define(param);
    }
    resolve(function->_body);
    end_scope();
    _current_function = enclosing_function;
}