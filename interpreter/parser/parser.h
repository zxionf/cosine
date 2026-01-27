#pragma once

#include "../ast/ast.h"
using namespace xel::ast;
#include "../lexer.h"
using namespace xel::lexer;
#include <sstream>
#include <functional>

namespace xel::parser
{
    class Parser
    {
        public:
            // 优先级
            enum Precedence
            {
                LOWEST,
                EQUALS,         // ==
                LESSGREATER,    // > or <
                SUM,            // +
                PRODUCT,        // *
                PREFIX,         // -X or !X
                CALL,           // myFunction(X)
            };

            Parser()  = default;
            Parser(const std::shared_ptr<Lexer>& lexer);
            ~Parser() = default;

            // 前缀表达式构建函数原型定义
            // typedef std::shared_ptr<Expression> (*prefix_parse_fn)(void);
            using prefix_parse_fn = std::shared_ptr<Expression>(Parser::*)(void);
            // 中缀表达式构建函数原型定义
            // typedef std::shared_ptr<Expression> (*infix_parse_fn)(const std::shared_ptr<Expression>& left);
            using infix_parse_fn = std::shared_ptr<Expression>(Parser::*)(const std::shared_ptr<Expression>& left);

            std::shared_ptr<Expression> parse_expression(int precedence);

            // prefix
            std::shared_ptr<Expression> parse_integer();
            std::shared_ptr<Expression> parse_grouped_expression();

            // infix
            std::shared_ptr<Expression> parse_infix(const std::shared_ptr<Expression>& left);

            // program
            std::shared_ptr<Program> parse_program();
            std::shared_ptr<Statement> parse_statement();
            std::shared_ptr<ExpressionStatement> parse_expression_statement();

            void no_prefix_parse_fn_error(Token::Type type);
            std::list<std::string>& get_errors();

            void next_token();
            bool cur_token_is(Token::Type type) const;
            bool peek_token_is(Token::Type type) const;
            bool expect_peek(Token::Type type);
            void peek_error(Token::Type type);

            int cur_precedence() const;
            int peek_precedence() const;

        private:
            std::shared_ptr<Lexer> _lexer;
            Token _cur_token;
            Token _peek_token;
            std::list<std::string> _errors;
            static std::map<Token::Type, Precedence> _precedences;
            static std::map<Token::Type, prefix_parse_fn> _prefix_parse_fns;
            static std::map<Token::Type, infix_parse_fn> _infix_parse_fns;
    };
}
