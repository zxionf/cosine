#include <iostream>
#include <lexer.h>

int main(){

    auto lexer = new xel::lexer::Lexer("test.xel");
    while(true){
        auto token = lexer->next_token();
        std::cout << token.get_name() << ": " << token.get_literal() << std::endl;
        if(token.get_type() == xel::token::Token::Type::TOKEN_EOF)
            break;
    }
    return 0;
}