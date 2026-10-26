#include "property.hpp"

namespace xel::anim
{
PropertyRegistry::PropertyRegistry()
{
    add("position", prop::Position, PropertyKind::Vec2);
    add("rotation", prop::Rotation, PropertyKind::Float);
    add("scale", prop::Scale, PropertyKind::Vec2);
    add("visible", prop::Visible, PropertyKind::Bool);
    add("size", prop::Size, PropertyKind::Vec2);
    add("color", prop::Color, PropertyKind::Vec4);
    add("text", prop::Text, PropertyKind::String);
    add("text_size", prop::TextSize, PropertyKind::Float);
    add("zoom", prop::Zoom, PropertyKind::Float);
    add("texture_id", prop::TextureId, PropertyKind::UInt);
}

void PropertyRegistry::add(std::string name, PropertyId id, PropertyKind kind)
{
    by_name_.emplace(name, id);
    by_id_.emplace(id, name);
    kinds_.emplace(id, kind);
}

PropertyId PropertyRegistry::id_of(std::string_view name) const
{
    const auto it = by_name_.find(std::string{name});
    return it == by_name_.end() ? kInvalidProperty : it->second;
}

std::string_view PropertyRegistry::name_of(PropertyId id) const
{
    const auto it = by_id_.find(id);
    return it == by_id_.end() ? std::string_view{} : std::string_view{it->second};
}

PropertyKind PropertyRegistry::kind_of(PropertyId id) const
{
    const auto it = kinds_.find(id);
    return it == kinds_.end() ? PropertyKind::Float : it->second;
}

const PropertyRegistry& builtin_properties()
{
    static const PropertyRegistry registry;
    return registry;
}

const char* to_string(PropertyKind kind)
{
    switch (kind) {
        case PropertyKind::Float: return "float";
        case PropertyKind::Vec2: return "vec2";
        case PropertyKind::Vec3: return "vec3";
        case PropertyKind::Vec4: return "vec4";
        case PropertyKind::Bool: return "bool";
        case PropertyKind::String: return "string";
        case PropertyKind::UInt: return "uint";
    }
    return "unknown";
}
} // namespace xel::anim
