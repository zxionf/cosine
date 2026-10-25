#include "events/key_code.hpp"

#include <cassert>
#include <iostream>

int main()
{
    using namespace xel::event;

    const Modifier both = Modifier::Shift | Modifier::Control;
    assert(has_modifier(both, Modifier::Shift));
    assert(has_modifier(both, Modifier::Control));
    assert(!has_modifier(both, Modifier::Alt));
    assert(!has_modifier(Modifier::None, Modifier::Shift));
    assert(KeyCode::Tab != KeyCode::Enter);

    std::cout << "key code tests passed\n";
    return 0;
}
