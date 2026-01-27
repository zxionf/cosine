#include "parser.h"
using namespace xel::parser;

std::shared_ptr<Program> Parser::parse_program(){
    std::shared_ptr<Program> program = std::make_shared<Program>();
    while(!cur_token_is(Token::Type::TOKEN_EOF))
    {
        auto stmt = parse_statement();
        // auto stmt = parse_expression_statement();
        if(stmt)
            program->_statements.push_back(stmt);
        next_token();
    }
    return program;
}