#include "node.h"
using namespace xel::ast;

std::map<Node::Type, std::string> Node::_type_map = {
    {Node::Type::PROGRAM,       "Program"},
    {Node::Type::INFIX,         "Infix"},
    {Node::Type::EXPRESSION,    "Expression"},
    {Node::Type::INTEGER,       "Integer"},
    {Node::Type::FLOAT,         "Float"},
};

std::string Node::get_name() {
    auto it = _type_map.find(_type);
    if (it != _type_map.end())
        return it->second;
    else return "?[node]?";
}