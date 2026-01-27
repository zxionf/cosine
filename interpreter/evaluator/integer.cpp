#include "evaluator.h"
using namespace xel::evaluator;

std::shared_ptr<Object> Evaluator::eval_integer(const std::shared_ptr<ast::Integer>& node){
    return new_integer(node->_value);
}