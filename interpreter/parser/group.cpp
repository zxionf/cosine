#include "parser.h"
using namespace xel::parser;

std::shared_ptr<Expression> Parser::parse_grouped_expression()
{
    next_token();
    auto expression = parse_expression(LOWEST);
    if (!expect_peek(Token::Type::RPAREN))
        return nullptr;
    return expression;
}