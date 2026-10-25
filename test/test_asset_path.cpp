#include "backend/asset_path.hpp"

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>

int main()
{
    using namespace xel::backend;

    const auto font = find_asset("fonts/default8.png");
    assert(font.found);
    assert(!font.searched.empty());

    const auto missing = find_asset("fonts/does-not-exist.png");
    assert(!missing.found);
    assert(!missing.searched.empty());
    const std::string message = format_asset_error(missing);
    assert(message.find("[Asset] file not found: fonts/does-not-exist.png") != std::string::npos);
    assert(message.find("searched:") != std::string::npos);
    assert(message.find("COSINE_ASSET_DIR") != std::string::npos);

    bool threw = false;
    try {
        (void)require_asset("fonts/does-not-exist.png");
    } catch (const std::runtime_error& e) {
        threw = true;
        assert(std::string(e.what()).find("[Asset]") != std::string::npos);
    }
    assert(threw);

    std::cout << "asset path tests passed\n";
    return 0;
}
