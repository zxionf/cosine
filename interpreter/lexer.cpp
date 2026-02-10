#include "lexer.h"
using namespace xel;
#include "xel_error.h"

#include <fstream>
#include <sstream>
#include <regex>

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

    tokens.emplace_back(Token::Type::TOKEN_EOF, "", nullptr, line);
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
            if (next_char_is('/')) // 单行注释
                while (peek_char() != '\n' && !is_at_end()) next_char();
            else
                add_token(Token::Type::SLASH);
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
            else xel::error::error(line, "Unexpected character.");
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
    add_token(type, nullptr);
}

void Lexer::add_token(Token::Type type, const var& literal) {
    std::string text = source.substr(start, current-start); // start , length
    tokens.emplace_back(Token(type, text, literal, line));
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
        xel::error::error(line, "Unterminated string.");
        return;
    }

    // The closing ".
    next_char();

    // 获取字符串
    std::string value = source.substr(start + 1, current - 1 - start - 1);
    std::regex ln{R"(\\n)"};
    value = std::regex_replace(value, ln, "\n");
    add_token(Token::Type::STRING, value);
}

bool Lexer::is_digit(char c) {
    return c >= '0' && c <= '9';
}

void Lexer::read_number() {
    while (is_digit(peek_char())) next_char();

    // 小数部分
    if (peek_char() == '.' && is_digit(peek_next_char())) {
      // Consume the "."
      next_char();

      while (is_digit(peek_char())) next_char();
    }
    std::string text = source.substr(start, current - start);
    add_token(Token::Type::NUMBER, std::stod(text));    // 默认转成double
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