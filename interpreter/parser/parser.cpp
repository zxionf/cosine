#include "parser.h"
using namespace xel::parser;

std::map<Token::Type, Parser::Precedence> Parser::_precedences = {
    {Token::Type::PLUS,       SUM},
    {Token::Type::MINUS,      SUM},
    {Token::Type::ASTERISK,   PRODUCT},
    {Token::Type::SLASH,      PRODUCT},
    // {Token::Type::PLUS,     3},
    // {Token::Type::MINUS,    3},
    // {Token::Type::SLASH,    4},
};

std::map<Token::Type, Parser::prefix_parse_fn> Parser::_prefix_parse_fns = {
    // {Token::Type::IDENTIFIER, &Parser::parse_identifier},
    {Token::Type::INTEGER,    &Parser::parse_integer},
    {Token::Type::LPAREN,     &Parser::parse_grouped_expression},
};
std::map<Token::Type, Parser::infix_parse_fn> Parser::_infix_parse_fns = {
    {Token::Type::PLUS,       &Parser::parse_infix},
    {Token::Type::MINUS,      &Parser::parse_infix},
    {Token::Type::ASTERISK,   &Parser::parse_infix},
    {Token::Type::SLASH,      &Parser::parse_infix},
};

Parser::Parser(const std::shared_ptr<Lexer>& lexer) :_lexer(lexer)
{
    next_token();
    next_token();
}

void Parser::next_token()
{
    _cur_token = _peek_token;
    _peek_token = _lexer->next_token();
}

bool Parser::cur_token_is(Token::Type type) const {
    return _cur_token.get_type() == type;
}
bool Parser::peek_token_is(Token::Type type) const{
    return _peek_token.get_type() == type;
}
bool Parser::expect_peek(Token::Type type) {
    if(peek_token_is(type))
    {
        next_token();
        return true;
    }else
    {
        peek_error(type);
        return false;
    }

}

int Parser::cur_precedence() const
{
    auto it = _precedences.find(_cur_token.get_type());
    if(it != _precedences.end())
        return it->second;
    else return LOWEST;
}
int Parser::peek_precedence() const
{
    auto it = _precedences.find(_peek_token.get_type());
    if(it != _precedences.end())
        return it->second;
    else return LOWEST;
}

void Parser::peek_error(Token::Type type) {
    std::stringstream ss;
    ss << "expected next token to be " << type << ", got " << _peek_token.get_type();
    _errors.push_back(ss.str());
}

void Parser::no_prefix_parse_fn_error(Token::Type type){
    std::stringstream ss;
    ss << "no prefix parse function for " << type;
    _errors.push_back(ss.str());
}

std::list<std::string>& Parser::get_errors() {
    return _errors;
}