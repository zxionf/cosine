#include "evaluator.h"
using namespace xel::evaluator;

bool Evaluator::is_error(const std::shared_ptr<Object>& obj){
    return obj->get_type() == Object::Type::ERROR;
}

std::shared_ptr<Object> Evaluator::new_error(const char* format, ...){
    char buffer[1024] = {0};
    va_list args_ptr;
    va_start(args_ptr, format);
    vsnprintf(buffer, sizeof(buffer), format, args_ptr);
    va_end(args_ptr);

    std::shared_ptr<Error> err = std::make_shared<Error>(buffer);
    return err;
}

std::shared_ptr<Object> Evaluator::new_integer(int32_t value){
    return Object::new_integer(value);
}

std::shared_ptr<Object> Evaluator::eval(const std::shared_ptr<ast::Node>& node){
    switch (node->get_type())
    {
        case Node::Type::PROGRAM:
        {
            auto s = std::dynamic_pointer_cast<ast::Program>(node);
            return eval_program(s->_statements);
        }
        case Node::Type::EXPRESSION_STATEMENT:
        {
            auto s = std::dynamic_pointer_cast<ast::ExpressionStatement>(node);
            return eval(s->_expression);
        }
        case Node::Type::INTEGER:
        {
            auto s = std::dynamic_pointer_cast<ast::Integer>(node);
            return eval_integer(s);
        }
        case Node::Type::INFIX:
        {
            auto s = std::dynamic_pointer_cast<ast::Infix>(node);
            auto left = eval(s->_left);
            if(is_error(left))
                return left;
            auto right = eval(s->_right);
            if(is_error(right))
                return right;
            return eval_infix(s->_operator, left, right);
        }
        default:
            return new_error("node type error");
    }
}