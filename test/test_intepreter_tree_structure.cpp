#include <iostream>
#include <lexer.h>
#include <ast/printer.h>
#include <parser/parser.h>
using namespace xel;

int main(){

    std::shared_ptr<Lexer> lexer = std::make_shared<Lexer>("test.xel");
    std::shared_ptr<Parser> parser = std::make_shared<Parser>(lexer->get_tokens());
    
    std::shared_ptr<Expr> expr = parser->parse();

    ASTPrinter printer;
    std::cout << printer.print(expr) << std::endl;

    // std::shared_ptr<Program> program = parser->parse_program();
    // std::cout << program->get_name() << std::endl;
    // std::cout << program->to_string() << std::endl;
    return 0;
}