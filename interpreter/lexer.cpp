#include "lexer.h"
using namespace xel::lexer;

#include <fstream>
#include <sstream>

Lexer::Lexer(const std::string& file_path) :input(""), ch(0), pos(0), next_pos(0)
{
    std::ifstream ifs(file_path);
    if (!ifs.good())
        throw std::runtime_error("File not found");
    std::ostringstream buffer;
    buffer << ifs.rdbuf();
    input = buffer.str();
    inputlen = input.length();
}

Lexer::Lexer(const char* buffer, size_t size) :ch(0), pos(0), next_pos(0) {
    input.assign(buffer, size);
    inputlen = input.length();
}

Token Lexer::next_token(){ 
    read_char();
    skip_whitespace();
    switch (ch) { 
        case ';':
            return Token(Token::Type::SEMICOLON, {ch});
        case '(':
            return Token(Token::Type::LPAREN, {ch});
        case ')':
            return Token(Token::Type::RPAREN, {ch});
        case ',':
            return Token(Token::Type::COMMA, {ch});
        case '.':
            return Token(Token::Type::DOT, {ch});
        case '+':
            return new_token(Token::Type::PLUS, {ch});
        case '-':
            return new_token(Token::Type::MINUS, {ch});
        case '*':
            return new_token(Token::Type::ASTERISK, {ch});
        case '/':
            return new_token(Token::Type::SLASH, {ch});
        case '=':
            return new_token(Token::Type::ASSIGN, {ch});
        case '!':
        case '&':
        case '|':
        case '^':
        case '~':
        case ':':
        case '\0':
            return new_token(Token::Type::TOKEN_EOF, {});
        default:
            if(is_digit(ch)){
                std::string number = read_number();
                unread_char();
                if(!next_char_is_letter()) return new_token(Token::Type::INTEGER, number);
                else return new_token(Token::Type::ILLEGAL, "error reading number");
            }
            else if(is_letter(ch)){
                std::string identifier = read_identifier();
                unread_char();
                Token::Type keyword = Token::find_keyword(identifier);
                return new_token(keyword, identifier);
            }
            else return new_token(Token::Type::ILLEGAL, {ch});
    }
}

void Lexer::skip_whitespace(){
    while (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
    {
        read_char();
    }
}
void Lexer::read_char(){
    if (next_pos >= inputlen)
    {
        ch = '\0';
        // return;
    }else
    {
        ch = input[next_pos];
    }
    pos = next_pos;
    ++next_pos;
}
void Lexer::unread_char(){
    next_pos = pos;
    --pos;
}
bool Lexer::is_digit(char ch){
    return ch >= '0' && ch <= '9';
}
bool Lexer::is_letter(char ch){
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_';
}
std::string Lexer::read_number(){
    int pos_ = pos;
    while(is_digit(ch)){
        read_char();
    }
    return input.substr(pos_, pos - pos_);
}
std::string Lexer::read_identifier(){
    int pos_ = pos;
    while(is_letter(ch) ||  is_digit(ch)){
        read_char();
    }
    return input.substr(pos_, pos - pos_);
}
bool Lexer::next_char_is_letter(){
    if(next_pos >= inputlen) return false;
    return is_letter(input[next_pos]);
}
Token Lexer::new_token(Token::Type type, const std::string& literal){
    return Token(type, literal);
}