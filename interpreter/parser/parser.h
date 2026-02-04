#pragma once


#include "../lexer.h"
#include "../ast/expr.h"
using namespace xel;

#include <memory>

namespace xel
{
    class Parser
    {
        public:
            Parser()  = default;
            Parser(std::list<Token> tokens) :_tokens(std::move(tokens)), _current(tokens.begin()) {}
            ~Parser() = default;

        private:
            std::shared_ptr<Expr> parse();
            std::shared_ptr<Expr> expression();
            std::shared_ptr<Expr> equality();

            template<typename... Args>
            bool match(Args... token_types);
            bool check(Token::Type type);

            Token advance();
            bool is_at_end();
            Token peek();
            Token previous();

            std::shared_ptr<Expr> comparison();
            std::shared_ptr<Expr> term();
            std::shared_ptr<Expr> factor();
            std::shared_ptr<Expr> unary();
            std::shared_ptr<Expr> primary();

            void synchronize();

            // 错误处理
            Token consume(Token::Type type, const std::string& message);
            std::runtime_error error(Token token, const std::string& message);

        public:
            std::list<Token> _tokens;
            std::list<Token>::iterator _current;
    };
}