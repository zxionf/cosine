#pragma once

#include "node.h"
#include <cstdint>

namespace xel::ast
{
    class Integer : public Expression
    {
    public:
        Integer() : Expression(Type::INTEGER) {}
        ~Integer() = default;

        virtual std::string to_string() override
        {
            // 返回名称和字面量
            return "{" + get_name() + ":" + std::to_string(_value) + "}";
        }

    public:
        int32_t _value;
    };
}
