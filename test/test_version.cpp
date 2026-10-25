#include "xel/version.hpp"

#include <cassert>
#include <cstring>
#include <iostream>
#include <string_view>

int main()
{
    const std::string_view version = xel::version_string();
    assert(std::strcmp(xel::kProjectName, "cosine") == 0);
    assert(!version.empty());

    int dots = 0;
    for (char ch : version) {
        if (ch == '.') {
            ++dots;
            continue;
        }
        assert(ch >= '0' && ch <= '9');
    }
    assert(dots == 2);

    std::cout << "version tests passed: " << version << "\n";
    return 0;
}
