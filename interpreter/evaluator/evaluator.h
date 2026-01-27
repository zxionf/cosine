#pragma once

#include "../ast/ast.h"
using namespace xel::ast;

#include "../object/header.h"
using namespace xel::object;

namespace xel::evaluator
{
    class Evaluator
    {
        public:
            Evaluator() = default;
            ~Evaluator() = default;

            bool is_error(const std::shared_ptr<Object>& obj);
            std::shared_ptr<Object> new_error(const char* format, ...);
            std::shared_ptr<Object> new_integer(int32_t value);

            // 对各种类型求值
            std::shared_ptr<Object> eval(const std::shared_ptr<ast::Node>& node);
            std::shared_ptr<Object> eval_program(const std::list<std::shared_ptr<ast::Statement>>& statements);
            std::shared_ptr<Object> eval_integer(const std::shared_ptr<ast::Integer>& node);

            // infix
            std::shared_ptr<Object> eval_infix(const std::string& op, const std::shared_ptr<Object>& left, const std::shared_ptr<Object>& right);
            std::shared_ptr<Object> eval_integer_infix(const std::string& op, const std::shared_ptr<Object>& left, const std::shared_ptr<Object>& right);
    };
}