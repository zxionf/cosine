#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>

namespace xel::anim
{
using PropertyId = std::uint32_t;
inline constexpr PropertyId kInvalidProperty = 0;

// 类型擦除的属性值：覆盖节点上需要被动画 / 序列化的字段。
using PropertyValue =
    std::variant<float, glm::vec2, glm::vec3, glm::vec4, bool, std::string, std::uint32_t>;

enum class PropertyKind
{
    Float,
    Vec2,
    Vec3,
    Vec4,
    Bool,
    String,
    UInt,
};

// 内置属性 id 是编译期常量：Scene 的分发用 switch，不需要查表。
namespace prop
{
inline constexpr PropertyId Position = 1;
inline constexpr PropertyId Rotation = 2;
inline constexpr PropertyId Scale = 3;
inline constexpr PropertyId Visible = 4;
inline constexpr PropertyId Size = 5;
inline constexpr PropertyId Color = 6;
inline constexpr PropertyId Text = 7;
inline constexpr PropertyId TextSize = 8;
inline constexpr PropertyId Zoom = 9;
inline constexpr PropertyId TextureId = 10;
} // namespace prop

// 名字 ↔ id ↔ 类型 的注册表；内置属性在构造时预注册。
// id 固定不变，注册表负责稳定名称（序列化 / M5 脚本按名查找）。
class PropertyRegistry
{
public:
    PropertyRegistry();

    PropertyId id_of(std::string_view name) const;
    std::string_view name_of(PropertyId id) const;
    PropertyKind kind_of(PropertyId id) const;

private:
    void add(std::string name, PropertyId id, PropertyKind kind);

    std::unordered_map<std::string, PropertyId> by_name_;
    std::unordered_map<PropertyId, std::string> by_id_;
    std::unordered_map<PropertyId, PropertyKind> kinds_;
};

// 进程内共享的内置注册表。
const PropertyRegistry& builtin_properties();

// 类型名（调试与序列化用）。
const char* to_string(PropertyKind kind);
} // namespace xel::anim
