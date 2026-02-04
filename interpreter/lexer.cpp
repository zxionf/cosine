#include "lexer.h"
using namespace xel::lexer;

#include <fstream>
#include <sstream>

std::map<std::string, Token::Type> Lexer::keywords = {
    {"and",     Token::Type::AND},
    {"class",   Token::Type::CLASS},
    {"else",    Token::Type::ELSE},
    {"false",   Token::Type::FALSE},
    {"for",     Token::Type::FOR},
    {"fun",     Token::Type::FUN},
    {"if",      Token::Type::IF},
    {"nil",     Token::Type::NIL},
    {"or",      Token::Type::OR},
    {"print",   Token::Type::PRINT},
    {"return",  Token::Type::RETURN},
    {"super",   Token::Type::SUPER},
    {"this",    Token::Type::THIS},
    {"true",    Token::Type::TRUE},
    {"var",     Token::Type::VAR},
    {"while",   Token::Type::WHILE},
};

Lexer::Lexer(const std::string& file_path) :source("")
{
    std::ifstream ifs(file_path);
    if (!ifs.good())
        throw std::runtime_error("File not found");
    std::ostringstream buffer;
    buffer << ifs.rdbuf();
    source = buffer.str();
    // inputlen = source.length();
}

Lexer::Lexer(const char* buffer, size_t size) {
    source.assign(buffer, size);
    // inputlen = source.length();
}

std::list<Token> Lexer::get_tokens()
{
    while (!is_at_end()) {
      // We are at the beginning of the next lexeme.
      start = current;
      get_token();
    }

    // TODO : optimize Object()
    tokens.push_back(Token(Token::Type::TOKEN_EOF, "", Object(), line));
    return tokens;
}

bool Lexer::is_at_end() {
    return current >= source.length();
}

void Lexer::get_token()
{
    char c = next_char();
    switch (c) {
        case '(': add_token(Token::Type::LEFT_PAREN); break;
        case ')': add_token(Token::Type::RIGHT_PAREN); break;
        case '{': add_token(Token::Type::LEFT_BRACE); break;
        case '}': add_token(Token::Type::RIGHT_BRACE); break;
        case ',': add_token(Token::Type::COMMA); break;
        case '.': add_token(Token::Type::DOT); break;
        case '-': add_token(Token::Type::MINUS); break;
        case '+': add_token(Token::Type::PLUS); break;
        case ';': add_token(Token::Type::SEMICOLON); break;
        case '*': add_token(Token::Type::STAR); break;
        case '!': add_token(next_char_is('=') ? Token::Type::BANG_EQUAL : Token::Type::BANG); break;
        case '=': add_token(next_char_is('=') ? Token::Type::EQUAL_EQUAL : Token::Type::EQUAL); break;
        case '<': add_token(next_char_is('=') ? Token::Type::LESS_EQUAL : Token::Type::LESS); break;
        case '>': add_token(next_char_is('=') ? Token::Type::GREATER_EQUAL : Token::Type::GREATER); break;
        case '/':
            if (next_char_is('/')) {
            // 单行注释
            while (peek_char() != '\n' && !is_at_end()) next_char();
            } else {
            add_token(Token::Type::SLASH);
            }
            break;
        case ' ':
        case '\r':
        case '\t':
            // 跳过空白字符
            break;
        case '\n':
            line++;
            break;
        case '"': read_string(); break;
        case 'o':
            if (next_char_is('r'))
                add_token(Token::Type::OR);
            break;
        default:
            if (is_digit(c)) read_number();
            else if (is_alpha(c)) read_identifier();
            else std::printf("Unexpected character at line %d\n", line);
            break;
    }
}

// TODO : optimize
char Lexer::next_char() {
    if (is_at_end())
        return '\0';
    return source[current++];
}

// TODO : optimize
void Lexer::add_token(Token::Type type) {
    add_token(type, Object());
}

void Lexer::add_token(Token::Type type, Object literal) {
    std::string text = source.substr(start, current-start); // start , length
    tokens.push_back(Token(type, text, literal, line));
}

bool Lexer::next_char_is(char expected) {
    if (is_at_end()) return false;
    if (source[current] != expected) return false;
    current++;
    return true;
}

char Lexer::peek_char() {
    if (is_at_end()) return '\0';
    return source[current];
}

void Lexer::read_string() {
    while (peek_char() != '"' && !is_at_end()) {
      if (peek_char() == '\n') line++;
      next_char();
    }

    if (is_at_end()) {
      std::printf("Unterminated string at line %d\n", line);
      return;
    }

    // The closing ".
    next_char();

    // Trim the surrounding quotes.
    std::string value = source.substr(start + 1, current - 1 - start - 1);
    add_token(Token::Type::STRING, value);
}

bool Lexer::is_digit(char c) {
    return c >= '0' && c <= '9';
}

void Lexer::read_number() {
    while (is_digit(peek_char())) next_char();

    // Look for a fractional part.
    if (peek_char() == '.' && is_digit(peek_next_char())) {
      // Consume the "."
      next_char();

      while (is_digit(peek_char())) next_char();
    }

    add_token(Token::Type::NUMBER, source.substr(start, current - start));
}

char Lexer::peek_next_char() {
    if (current + 1 >= source.length()) return '\0';
    return source[current + 1];
}

void Lexer::read_identifier() {
    while (is_alpha_numeric(peek_char())) next_char();

    std::string text = source.substr(start, current - start);
    auto it = keywords.find(text);
    if (it == keywords.end())
        add_token(Token::Type::IDENTIFIER);
    else add_token(it->second);
  }

bool Lexer::is_alpha(char c ){
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
            c == '_';
}

bool Lexer::is_alpha_numeric(char c){
    return is_alpha(c) || is_digit(c);
}

// Token Lexer::next_token(){ 
//     read_char();
//     skip_whitespace();
//     switch (ch) { 
//         case ';':
//             return Token(Token::Type::SEMICOLON, {ch});
//         case '(':
//             return Token(Token::Type::LPAREN, {ch});
//         case ')':
//             return Token(Token::Type::RPAREN, {ch});
//         case ',':
//             return Token(Token::Type::COMMA, {ch});
//         case '.':
//             return Token(Token::Type::DOT, {ch});
//         case '+':
//             return new_token(Token::Type::PLUS, {ch});
//         case '-':
//             return new_token(Token::Type::MINUS, {ch});
//         case '*':
//             return new_token(Token::Type::ASTERISK, {ch});
//         case '/':
//             return new_token(Token::Type::SLASH, {ch});
//         case '=':
//             return new_token(Token::Type::ASSIGN, {ch});
//         case '!':
//         case '&':
//         case '|':
//         case '^':
//         case '~':
//         case ':':
//         case '\0':
//             return new_token(Token::Type::TOKEN_EOF, {});
//         default:
//             if(is_digit(ch)){
//                 std::string number = read_number();
//                 unread_char();
//                 if(!next_char_is_letter()) return new_token(Token::Type::INTEGER, number);
//                 else return new_token(Token::Type::ILLEGAL, "error reading number");
//             }
//             else if(is_letter(ch)){
//                 std::string identifier = read_identifier();
//                 unread_char();
//                 Token::Type keyword = Token::find_keyword(identifier);
//                 return new_token(keyword, identifier);
//             }
//             else return new_token(Token::Type::ILLEGAL, {ch});
//     }
// }

// void Lexer::skip_whitespace(){
//     while (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
//     {
//         read_char();
//     }
// }
// void Lexer::read_char(){
//     if (next_pos >= inputlen)
//     {
//         ch = '\0';
//         // return;
//     }else
//     {
//         ch = input[next_pos];
//     }
//     pos = next_pos;
//     ++next_pos;
// }
// void Lexer::unread_char(){
//     next_pos = pos;
//     --pos;
// }
// bool Lexer::is_digit(char ch){
//     return ch >= '0' && ch <= '9';
// }
// bool Lexer::is_letter(char ch){
//     return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_';
// }
// std::string Lexer::read_number(){
//     int pos_ = pos;
//     while(is_digit(ch)){
//         read_char();
//     }
//     return input.substr(pos_, pos - pos_);
// }
// std::string Lexer::read_identifier(){
//     int pos_ = pos;
//     while(is_letter(ch) ||  is_digit(ch)){
//         read_char();
//     }
//     return input.substr(pos_, pos - pos_);
// }
// bool Lexer::next_char_is_letter(){
//     if(next_pos >= inputlen) return false;
//     return is_letter(input[next_pos]);
// }
// Token Lexer::new_token(Token::Type type, const std::string& literal){
//     return Token(type, literal);
// }