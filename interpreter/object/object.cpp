#include "object.h"
#include "integer.h"
#include "error.h"
using namespace xel::object;

std::map<Object::Type, std::string> Object::_type_map = {
    {Object::Type::ERROR,   "error"},
    {Object::Type::INTEGER, "integer"},
};

std::string Object::get_name() const {
    auto it = _type_map.find(_type);
    return it != _type_map.end() ? it->second : "unknown";
}

std::shared_ptr<Object> Object::new_error(const char* format, ...){
    char buffer[1024] = {0};
    va_list args_ptr;
    va_start(args_ptr, format);
    vsnprintf(buffer, sizeof(buffer), format, args_ptr);
    va_end(args_ptr);

    std::shared_ptr<Error> err = std::make_shared<Error>(buffer);
    return err;
}
std::shared_ptr<Object> Object::new_integer(int32_t value){
    return std::make_shared<Integer>(value);
}