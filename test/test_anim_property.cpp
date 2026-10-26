#include "anim/property.hpp"
#include "anim/scene.hpp"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <string>

namespace
{
using namespace xel;

void test_registry_names()
{
    const auto& reg = anim::builtin_properties();
    assert(reg.id_of("position") == anim::prop::Position);
    assert(reg.id_of("rotation") == anim::prop::Rotation);
    assert(reg.id_of("text") == anim::prop::Text);
    assert(reg.id_of("nope") == anim::kInvalidProperty);

    assert(reg.name_of(anim::prop::Position) == "position");
    assert(reg.name_of(9999).empty());
    assert(reg.kind_of(anim::prop::Color) == anim::PropertyKind::Vec4);
    assert(reg.kind_of(anim::prop::Position) == anim::PropertyKind::Vec2);
    assert(std::string(anim::to_string(anim::PropertyKind::Vec2)) == "vec2");
}

void test_get_set_all()
{
    scene::Scene s;
    const auto id = s.create(scene::NodeType::Text, "node");

    assert(s.set_property(id, anim::prop::Position, glm::vec2{10.0f, 20.0f}));
    assert(std::get<glm::vec2>(*s.get_property(id, anim::prop::Position)).x == 10.0f);

    assert(s.set_property(id, anim::prop::Rotation, 1.25f));
    assert(std::get<float>(*s.get_property(id, anim::prop::Rotation)) == 1.25f);

    assert(s.set_property(id, anim::prop::Scale, glm::vec2{2.0f, 3.0f}));
    assert(s.set_property(id, anim::prop::Visible, false));
    assert(!std::get<bool>(*s.get_property(id, anim::prop::Visible)));

    assert(s.set_property(id, anim::prop::Size, glm::vec2{100.0f, 50.0f}));
    assert(s.set_property(id, anim::prop::Color, glm::vec4{1.0f, 0.0f, 0.0f, 1.0f}));
    assert(s.set_property(id, anim::prop::Text, std::string{"hello"}));
    assert(std::get<std::string>(*s.get_property(id, anim::prop::Text)) == "hello");

    assert(s.set_property(id, anim::prop::TextSize, 22.0f));
    assert(s.set_property(id, anim::prop::Zoom, 1.5f));
    assert(s.set_property(id, anim::prop::TextureId, static_cast<std::uint32_t>(7)));

    // 写回节点字段本身。
    assert(s.find(id)->transform.position.x == 10.0f);
    assert(s.find(id)->text == "hello");
    assert(s.find(id)->texture_id == 7u);
    assert(!s.find(id)->visible);
}

void test_type_mismatch_rejected()
{
    scene::Scene s;
    const auto id = s.create(scene::NodeType::Shape);

    // position 是 vec2，传 float 必须失败。
    assert(!s.set_property(id, anim::prop::Position, 1.0f));
    // rotation 是 float，传 vec2 失败。
    assert(!s.set_property(id, anim::prop::Rotation, glm::vec2{1.0f, 2.0f}));
    // text 是 string，传 float 失败。
    assert(!s.set_property(id, anim::prop::Text, 1.0f));
    // 未知属性。
    assert(!s.set_property(id, 999, 1.0f));
    assert(!s.get_property(id, 999).has_value());

    // 不存在的节点。
    assert(!s.set_property(9999, anim::prop::Position, glm::vec2{1.0f, 2.0f}));
    assert(!s.get_property(9999, anim::prop::Position).has_value());
}

void test_defaults()
{
    scene::Scene s;
    const auto id = s.create(scene::NodeType::Group);

    assert(std::get<glm::vec2>(*s.get_property(id, anim::prop::Position)).x == 0.0f);
    assert(std::get<float>(*s.get_property(id, anim::prop::Rotation)) == 0.0f);
    assert(std::get<bool>(*s.get_property(id, anim::prop::Visible)));
    assert(std::get<std::string>(*s.get_property(id, anim::prop::Text)).empty());
    assert(std::get<std::uint32_t>(*s.get_property(id, anim::prop::TextureId)) == 0u);
}
} // namespace

int main()
{
    test_registry_names();
    test_get_set_all();
    test_type_mismatch_rejected();
    test_defaults();
    std::printf("test_anim_property: all passed\n");
    return 0;
}
