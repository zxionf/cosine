#include <iostream>
#include "lexer.h"
#include "parser/parser.h"
#include "evaluator/evaluator.h"
using namespace xel;

// repl
// read evaluate print loop

const std::string PROMPT = ">> ";

int main(){
    std::cout << "Xel Interpreter >_>" << std::endl;

    std::shared_ptr<Evaluator> evaluator = std::make_shared<Evaluator>();
    while(true)
    {
        std::cout << PROMPT;
        std::string input;
        std::getline(std::cin, input);

        // 词法分析
        std::shared_ptr<Lexer> lexer = std::make_shared<Lexer>(input.c_str(), input.size());
        // 语法分析
        std::shared_ptr<Parser> parser = std::make_shared<Parser>(lexer->get_tokens());
        // 解释
        evaluator->interpret(parser->parse());
        std::cout << std::endl;
    }

    
    return 0;
}