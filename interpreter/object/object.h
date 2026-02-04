#pragma once

#include <map>
#include <string>
#include <memory>
#include <cstdarg>
#include <cstdint>

namespace xel
{
    class Object
    {
        public:
            enum Type
            {
                ERROR,
                INTEGER,
                STRING,
            };

            Object() = default;
            Object(Type type) :_type(type){}
            Object(const std::string& literal): _type(STRING), _value(literal) {}
            // virtual ~Object() = default;

            Type get_type() const { return _type; }
            std::string get_name() const;
            // virtual std::string to_string() = 0;

        public:
            Type _type;
            std::string _value;
            static std::map<Type, std::string> _type_map;
    };
}