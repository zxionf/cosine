#pragma once

#include "../token.h"
#include <unordered_map>

namespace xel
{
    class Environment
    {
        public:
            Environment() :_enclosing(nullptr) {}
            Environment(Environment* enclosing) :_enclosing(enclosing) {}
            void define(const std::string& name, const var& value);
            var get(const Token& name);
            void assign(const Token& name, const var& value);
            
        private:
            Environment* _enclosing;
            std::unordered_map<std::string, var> _values;
    };
}