#pragma once

#include "../token.h"
#include <unordered_map>

namespace xel
{
    class Environment
    {
        public:
            Environment() = default;
            void define(const std::string& name, const std::any& value);
            std::any get(const Token& name);
            void assign(const Token& name, const std::any& value);
            
            std::unordered_map<std::string, std::any> _values;
    };
}