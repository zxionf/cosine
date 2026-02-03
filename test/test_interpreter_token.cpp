#include <iostream>
#include <lexer.h>

int main(){

    auto lexer = new xel::lexer::Lexer("test.xel");
    lexer->get_tokens();
    for(auto token : lexer->get_tokens()){
        std::cout << token.to_string() << std::endl;
    }
    return 0;
}