#pragma once

#include "node.h"

namespace xel::ast
{
    class Program : public Statement
    {
    public:
        Program() :Statement(Type::PROGRAM) {}
        ~Program() = default;

        virtual std::string to_string() override
        {
            std::string result;
            for (auto& statement : _statements)
            {
                result += statement->to_string();
            }
            return result;
        }

        std::list<std::shared_ptr<Statement>> _statements;
    };
}