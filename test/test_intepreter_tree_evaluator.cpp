#include <iostream>
#include <lexer.h>
#include <parser/parser.h>
#include <evaluator/evaluator.h>
using namespace xel;

int main(){
    // 词法分析
    std::shared_ptr<Lexer> lexer = std::make_shared<Lexer>("test.xel");
    // 语法分析
    std::shared_ptr<Parser> parser = std::make_shared<Parser>(lexer->get_tokens());
    Evaluator evaluator{};
    evaluator.interpret(parser->parse());
    
    
    return 0;
}