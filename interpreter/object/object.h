#pragma once

#include <map>
#include <string>
#include <memory>
#include <cstdarg>
#include <cstdint>

namespace xel::object
{
    class Object
    {
        public:
            enum Type
            {
                ERROR,
                INTEGER,
            };

            Object() = default;
            Object(Type type) :_type(type){}
            virtual ~Object() = default;

            Type get_type() const { return _type; }
            std::string get_name() const;
            virtual std::string to_string() = 0;

            static std::shared_ptr<Object> new_error(const char* format, ...);
            static std::shared_ptr<Object> new_integer(int32_t value);

        protected:
            Type _type;
            static std::map<Type, std::string> _type_map;
    };
}