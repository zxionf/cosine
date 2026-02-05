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
    else
        xel::runtime_error::error(name, "Undefined variable '" + name.get_lexeme() + "'.");
}