#include "parser.h"
using namespace xel::parser;

std::shared_ptr<Statement> Parser::parse_statement(){
    return parse_expression_statement();
}

std::shared_ptr<ExpressionStatement> Parser::parse_expression_statement(){
    std::shared_ptr<ExpressionStatement> statement = std::make_shared<ExpressionStatement>();
    statement->set_token(_cur_token);
    statement->_expression = parse_expression(LOWEST);
    while(peek_token_is(Token::Type::SEMICOLON))
        next_token();
    return statement;
}