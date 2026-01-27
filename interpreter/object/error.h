#include "object.h"

namespace xel::object
{
    class Error : public Object
    {
        public:
            Error() :Object(Type::ERROR) {}
            Error(const std::string& message) :Object(Type::ERROR), _message(message) {}
            ~Error() = default;

            virtual std::string to_string() override
            {
                return "ERROR: " + _message;
            }

        public:
            std::string _message;
    };
}