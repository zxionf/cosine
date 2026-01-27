#include "parser.h"
using namespace xel::parser;

std::shared_ptr<Expression> Parser::parse_expression(int precedence) {
    auto prefix = _prefix_parse_fns.find(_cur_token.get_type());
    if (prefix == _prefix_parse_fns.end()) {
        no_prefix_parse_fn_error(_cur_token.get_type());
        return nullptr;
    }
    std::shared_ptr<Expression> left = (this->*(prefix->second))();
    while(!peek_token_is(Token::Type::SEMICOLON) && precedence < peek_precedence())
    {
        auto infix = _infix_parse_fns.find(_peek_token.get_type());
        if(infix == _infix_parse_fns.end())
            return left;
        next_token();
        left = (this->*(infix->second))(left);
    }
    return left;
}