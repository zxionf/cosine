#include "object.h"

namespace xel::object
{
    class Integer : public Object
    {
        public:
            Integer() :Object(Type::INTEGER), _value(0) {}
            Integer(int32_t value) :Object(Type::INTEGER), _value(value) {}
            ~Integer() = default;

            virtual std::string to_string() override
            {
                return std::to_string(_value);
            }

            int32_t _value;
    };
}
