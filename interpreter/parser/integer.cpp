#include "parser.h"
using namespace xel::parser;

std::shared_ptr<Expression> Parser::parse_integer()
{
    std::shared_ptr<Integer> integer = std::make_shared<Integer>();
    integer->set_token(_cur_token);
    integer->_value = std::stoi(_cur_token.get_literal());
    return integer;
}