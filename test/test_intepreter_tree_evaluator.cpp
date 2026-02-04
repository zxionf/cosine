#include <iostream>
#include <lexer.h>
#include <parser/parser.h>
#include <evaluator/evaluator.h>
using namespace xel;

int main(){
    // 词法分析
    std::shared_ptr<Lexer> lexer = std::make_shared<Lexer>("test.xel");
    // 语法分析
    std::shared_ptr<Parser> parser = std::make_shared<Parser>(lexer);
    // 构建抽象语法树
    std::shared_ptr<Program> program = parser->parse_program();
    // 打印语法树
    std::cout << program->get_name() << std::endl;
    std::cout << program->to_string() << std::endl;
    // 打印错误
    auto errors = parser->get_errors();
    if(!errors.empty())
    for (auto error : errors) {
        std::cout << error << std::endl;
    }
    // 求值
    std::shared_ptr<Evaluator> evaluator = std::make_shared<Evaluator>();
    std::shared_ptr<Object> result = evaluator->eval(program);
    // if(result)
        // std::cout << result->to_string() << std::endl;
    // else std::cout << "NULL" << std::endl;
    return 0;
}