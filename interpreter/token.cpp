#include "token.h"
using namespace xel;

std::map<Token::Type, std::string> Token::type_map = {
    {Type::TOKEN_EOF,   "eof"},
    // {Type::ILLEGAL,     "illegal"},
    {Type::IDENTIFIER,  "identifier"},
    {Type::VAR,         "var"},
    {Type::NUMBER,     "number"},
    // {Type::FLOAT,       "float"},
    {Type::PLUS,        "+"},
    {Type::MINUS,       "-"},
    {Type::STAR,        "*"},
    {Type::SLASH,       "/"},
    // {Type::MOD,         "%"},
    {Type::LEFT_PAREN,  "("},
    {Type::RIGHT_PAREN, ")"},
    {Type::SEMICOLON,   ";"},
};

Token& Token::operator=(const Token& other){
    if (this == &other) return *this;
    type = other.type;
    literal = other.literal;
    return *this;
}

Token::Type Token::get_type() const {
    return type;
}

std::string Token::get_name() const {
    auto it = type_map.find(type);
    if (it != type_map.end())
        return it->second;
    else return "?[unknow_token_name]?";
}

std::string Token::get_lexeme() const {
    return lexeme;
}

std::any Token::get_literal() const {
    return literal;
}

int Token::get_line() const {
    return line;
}

std::string Token::to_string() const {
    return "Token(" + get_name() + ", " + get_lexeme() + ")";
}