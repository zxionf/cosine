#pragma once

#include "../lexer.h"
#include "../ast/stmt.h"

namespace xel
{
    class Parser
    {
        public:
            Parser()  = default;
            Parser(std::list<Token> tokens) :_tokens(std::move(tokens)), _current(_tokens.begin()) {}
            ~Parser() = default;

        public:
            // std::shared_ptr<Expr> parse();      // 解析
            std::list<std::shared_ptr<Stmt>> parse();       // 解析
        
        private:
            // 对于语句
            std::shared_ptr<Stmt> statement();              // 语句
            std::shared_ptr<Stmt> if_statement();           // if 语句
            std::shared_ptr<Stmt> while_statement();        // while 语句
            std::shared_ptr<Stmt> for_statement();          // for 语句
            std::shared_ptr<Stmt> print_statement();        // print 语句
            std::shared_ptr<Stmt> expression_statement();   // 表达式语句
            std::shared_ptr<Stmt> function(const std::string& kind); // 函数
            std::shared_ptr<Stmt> return_statement();       // 返回
            std::list<std::shared_ptr<Stmt>> block();       // 语句块
            std::shared_ptr<Stmt> declaration();            // 声明语句
            std::shared_ptr<Stmt> var_declaration();        // 变量声明语句

            // 对于表达式
            std::shared_ptr<Expr> expression();             // 表达式
            std::shared_ptr<Expr> equality();               // 等式
            std::shared_ptr<Expr> assignment();             // 赋值
            std::shared_ptr<Expr> logical_or();             // 或
            std::shared_ptr<Expr> logical_and();            // 与
            std::shared_ptr<Expr> call();                   // 调用
            std::shared_ptr<Expr> finish_call(std::shared_ptr<Expr> callee);

            template<typename... Type>
            bool match(Type... token_types);    // 匹配
            bool check(Token::Type type);       // match 的辅助函数

            Token& next_token();    // 移动到下一个 token
            bool   is_at_end();
            Token& peek_token();    // 返回当前 token
            Token& prev_token();    // 返回前一个 token

            std::shared_ptr<Expr> comparison();
            std::shared_ptr<Expr> term();
            std::shared_ptr<Expr> factor();
            std::shared_ptr<Expr> unary();
            std::shared_ptr<Expr> primary();

            void synchronize();

            // 错误处理
            Token& consume(Token::Type type, const std::string& message);
            std::runtime_error error(const Token& token, const std::string& message);

        public:
            std::list<Token> _tokens;
            std::list<Token>::iterator _current;
    };
}