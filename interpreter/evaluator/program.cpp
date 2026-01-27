#include "evaluator.h"
using namespace xel::evaluator;

std::shared_ptr<Object> Evaluator::eval_program(const std::list<std::shared_ptr<Statement>>& statements){
    std::shared_ptr<Object> result;
    for(auto& statement : statements)
    {
        result = eval(statement);
        if(is_error(result))
            break;
    }
    return result;
}