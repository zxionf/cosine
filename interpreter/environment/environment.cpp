#include "environment.h"
#include "xel_error.h"
using namespace xel;

void Environment::define(const std::string& name, const std::any& value) {
    _values.emplace(name, value);
}

std::any Environment::get(const Token& name) {
    auto it = _values.find(name.get_lexeme());
    if (it != _values.end())
        return it->second;
    if (_enclosing != nullptr)
        return _enclosing->get(name);
    throw xel::error::error_(name, "Undefined variable '" + name.get_lexeme() + "'.");
    return nullptr;
}

void Environment::assign(const Token& name, const std::any& value) {
    auto it = _values.find(name.get_lexeme());
    if (it != _values.end()) {
        // TODO : optimize
        it->second = value;
        return;
    }
    if (_enclosing != nullptr) {
        _enclosing->assign(name, value);
        return;
    }
    throw xel::error::error_(name, "Undefined variable '" + name.get_lexeme() + "'.");
}