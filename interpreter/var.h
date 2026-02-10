#pragma once

#include <memory>
#include <string>
#include <variant>

namespace xel
{
    class Callable;
    using var = std::variant
    <
        std::nullptr_t,
        bool,
        double,
        std::string,

        std::shared_ptr<Callable>
    >;

    template<class... Ts>
    struct overloaded : Ts... { using Ts::operator()...; };

    template<class... Ts>
    overloaded(Ts...) -> overloaded<Ts...>;

}
