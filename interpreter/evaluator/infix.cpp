#include "evaluator.h"
using namespace xel::evaluator;

std::shared_ptr<Object> Evaluator::eval_infix(const std::string& op, const std::shared_ptr<Object>& left, const std::shared_ptr<Object>& right){
    switch (left->get_type())
    {
    case Object::Type::INTEGER:
        if(right->get_type() == Object::Type::INTEGER)
            return eval_integer_infix(op, left, right);
        break;
    
    default:
        break;
    }
    return new_error("unknown operator: %s %s %s", left->to_string().c_str(), op.c_str(), right->to_string().c_str());
}

std::shared_ptr<Object> Evaluator::eval_integer_infix(const std::string& op, const std::shared_ptr<Object>& left, const std::shared_ptr<Object>& right){
    auto l = std::dynamic_pointer_cast<object::Integer>(left);
    auto r = std::dynamic_pointer_cast<object::Integer>(right);
    if(op == "+")
        return new_integer(l->_value + r->_value);
    else if(op == "-")
        return new_integer(l->_value - r->_value);
    else if(op == "*")
        return new_integer(l->_value * r->_value);
    else if(op == "/")
        return new_integer(l->_value / r->_value);
    return new_error("unknown operator: %s %s %s", left->get_name().c_str(), op.c_str(), right->get_name().c_str());
    // const char* b = op.c_str();
    // return new_error("operator.literal: %s", op.c_str());
}