#include <iostream>
#include <lexer.h>

int main(){

    xel::Lexer lexer("test.xel");
    for(auto token : lexer.get_tokens()){
        std::cout << token.to_string() << std::endl;
    }
    return 0;
}
