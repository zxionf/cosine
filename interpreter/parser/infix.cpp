#include "parser.h"
using namespace xel::parser;

std::shared_ptr<Expression> Parser::parse_infix(const std::shared_ptr<Expression>& left) {
    std::shared_ptr<Infix> infix = std::make_shared<Infix>();
    infix->set_token(_cur_token);
    infix->_left = left;
    infix->_operator = _cur_token.get_literal();
    int precedence = cur_precedence();
    next_token();
    infix->_right = parse_expression(precedence);
    return infix;
}
