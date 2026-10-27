#include "anim/serialize.hpp"

#include "anim/easing.hpp"
#include "anim/property.hpp"
#include "anim/scene.hpp"
#include "anim/timeline.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace
{
namespace ns = xel::scene;
namespace prop = xel::anim::prop;
using xel::anim::deserialize;
using xel::anim::Easing;
using xel::anim::Keyframe;
using xel::anim::Project;
using xel::anim::serialize;

constexpr float kEps = 1.0e-6f;

Project make_sample()
{
    Project p;
    const auto group = p.scene.create(ns::NodeType::Group, "group");
    auto* gnode = p.scene.find(group);
    gnode->transform.position = {10.0f, 20.0f};
    gnode->transform.rotation = 0.5f;
    gnode->transform.scale = {2.0f, 3.0f};

    const auto shape = p.scene.create(ns::NodeType::Shape, "shape");
    p.scene.set_parent(shape, group);
    auto* snode = p.scene.find(shape);
    snode->size = {40.0f, 30.0f};
    snode->color = {1.0f, 0.5f, 0.25f, 1.0f};
    snode->ellipse = true;

    const auto text = p.scene.create(ns::NodeType::Text, "");
    p.scene.set_parent(text, shape);
    auto* tnode = p.scene.find(text);
    tnode->text = "你好\nworld";
    tnode->text_size = 22.0f;
    tnode->visible = false;

    p.timeline.set_duration(3.5f);
    p.timeline.set_fps(30.0f);
    p.timeline.set_loop(false);
    p.timeline.set_time_scale(1.5f);

    auto& position = p.timeline.add_track(shape, prop::Position);
    position.add_key(Keyframe{0.0f, glm::vec2{0.0f, 0.0f}, Easing::Linear});
    Keyframe curved{2.0f, glm::vec2{100.0f, 50.0f}, Easing::CubicBezier};
    curved.custom = {0.2f, 0.6f, 0.8f, 0.1f};
    position.add_key(curved);

    auto& visible = p.timeline.add_track(group, prop::Visible);
    visible.add_key(Keyframe{0.0f, true, Easing::Linear});
    visible.add_key(Keyframe{1.0f, false, Easing::Linear});

    auto& label = p.timeline.add_track(text, prop::Text);
    label.add_key(Keyframe{0.5f, std::string{"phase 1"}, Easing::Linear});

    return p;
}

void test_round_trip()
{
    Project source = make_sample();
    const std::vector<std::uint8_t> bytes = serialize(source);
    assert(bytes.size() > 16);
    assert(std::memcmp(bytes.data(), "CSNP", 4) == 0);
    assert(bytes[8] == 1 && bytes[9] == 0); // major.minor

    Project restored;
    std::string error;
    const bool ok = deserialize(bytes, restored, &error);
    if (!ok) std::fprintf(stderr, "round-trip failed: %s\n", error.c_str());
    assert(ok);
    assert(error.empty());

    assert(restored.scene.size() == 4); // root + group + shape + text

    const auto root = static_cast<ns::NodeId>(1);
    const auto group = static_cast<ns::NodeId>(2);
    const auto shape = static_cast<ns::NodeId>(3);
    const auto text = static_cast<ns::NodeId>(4);

    assert(restored.scene.root() == root);

    const auto* r = restored.scene.find(root);
    const auto* g = restored.scene.find(group);
    const auto* s = restored.scene.find(shape);
    const auto* t = restored.scene.find(text);
    assert(r && g && s && t);
    assert(g->parent == root);

    assert(std::fabs(g->transform.position.x - 10.0f) < kEps);
    assert(std::fabs(g->transform.position.y - 20.0f) < kEps);
    assert(std::fabs(g->transform.rotation - 0.5f) < kEps);
    assert(std::fabs(g->transform.scale.x - 2.0f) < kEps);
    assert(std::fabs(g->transform.scale.y - 3.0f) < kEps);

    assert(std::fabs(s->size.x - 40.0f) < kEps);
    assert(std::fabs(s->color.y - 0.5f) < kEps);
    assert(s->ellipse);
    assert(s->parent == group);

    assert(t->text == "你好\nworld");
    assert(std::fabs(t->text_size - 22.0f) < kEps);
    assert(!t->visible);
    assert(t->parent == shape);

    // 父子关系与顺序（group 挂在 root 下）。
    assert(r->children.size() == 1 && r->children[0] == group);
    assert(g->children.size() == 1 && g->children[0] == shape);
    assert(s->children.size() == 1 && s->children[0] == text);

    // 时间轴元数据。
    assert(std::fabs(restored.timeline.duration() - 3.5f) < kEps);
    assert(std::fabs(restored.timeline.fps() - 30.0f) < kEps);
    assert(!restored.timeline.loop());
    assert(std::fabs(restored.timeline.time_scale() - 1.5f) < kEps);

    // 轨道与关键帧。
    assert(restored.timeline.tracks().size() == 3);
    const auto* position = restored.timeline.find_track(shape, prop::Position);
    assert(position && position->keys().size() == 2);
    assert(std::fabs(position->keys()[1].time - 2.0f) < kEps);
    assert(position->keys()[1].easing == Easing::CubicBezier);
    assert(std::fabs(position->keys()[1].custom.y2 - 0.1f) < kEps);
    assert(std::get<glm::vec2>(position->keys()[1].value).x == 100.0f);

    const auto* visible = restored.timeline.find_track(group, prop::Visible);
    assert(visible && visible->keys().size() == 2);
    assert(std::get<bool>(visible->keys()[0].value) == true);
    assert(std::get<bool>(visible->keys()[1].value) == false);

    const auto* label = restored.timeline.find_track(text, prop::Text);
    assert(label && label->keys().size() == 1);
    assert(std::get<std::string>(label->keys()[0].value) == "phase 1");
}

// 同一项目重复保存必须产生完全相同的字节。
void test_deterministic()
{
    Project source = make_sample();
    const auto first = serialize(source);
    Project restored;
    std::string error;
    assert(deserialize(first, restored, &error));
    const auto second = serialize(restored);
    assert(first == second);
}

// 空项目也能往返（0 节点、0 轨道）。
void test_empty_round_trip()
{
    Project empty;
    const auto bytes = serialize(empty);
    Project restored;
    std::string error;
    assert(deserialize(bytes, restored, &error));
    assert(restored.scene.size() == 1); // 只有默认 root
    assert(restored.scene.root() == 1);
    assert(restored.timeline.tracks().empty());
    assert(serialize(restored) == bytes);
}

void expect_fail(const std::vector<std::uint8_t>& bytes, const char* needle, int line)
{
    Project out;
    std::string error;
    const bool ok = deserialize(bytes, out, &error);
    if (ok || error.find(needle) == std::string::npos) {
        std::printf("line %d: expected error containing '%s', got '%s'\n", line, needle,
                    error.c_str());
        std::abort();
    }
}

#define EXPECT_FAIL(bytes, needle) expect_fail((bytes), (needle), __LINE__)

// 手工拼一个最小文件：只写节点表，便于构造非法层级。
std::vector<std::uint8_t> make_raw(const std::vector<std::pair<std::uint64_t, std::vector<std::uint64_t>>>& nodes)
{
    std::vector<std::uint8_t> out;
    auto u8 = [&out](std::uint8_t v) { out.push_back(v); };
    auto u32 = [&out](std::uint32_t v) {
        for (int i = 0; i < 4; ++i) out.push_back(static_cast<std::uint8_t>((v >> (8 * i)) & 0xFF));
    };
    auto u64 = [&u32](std::uint64_t v) {
        u32(static_cast<std::uint32_t>(v & 0xFFFFFFFFu));
        u32(static_cast<std::uint32_t>((v >> 32) & 0xFFFFFFFFu));
    };
    auto f32 = [&u32](float v) {
        std::uint32_t bits = 0;
        std::memcpy(&bits, &v, 4);
        u32(bits);
    };
    auto str = [&u32, &out](const char* s) {
        const auto len = static_cast<std::uint32_t>(std::strlen(s));
        u32(len);
        out.insert(out.end(), s, s + len);
    };

    out.insert(out.end(), {'C', 'S', 'N', 'P'});
    out.insert(out.end(), {0x0D, 0x0A, 0x1A, 0x0A});
    u8(1);
    u8(0);
    out.push_back(0);
    out.push_back(0); // flags
    u32(16);          // header_size

    u32(static_cast<std::uint32_t>(nodes.size()));
    for (const auto& [id, children] : nodes) {
        u64(id);
        u8(0); // Group
        u8(1); // visible
        f32(0.0f);
        f32(0.0f);
        f32(0.0f);
        f32(1.0f);
        f32(1.0f);
        f32(0.0f);
        f32(0.0f);
        for (int i = 0; i < 4; ++i) f32(1.0f);
        f32(16.0f);
        u32(0); // texture_id
        f32(1.0f);
        str("n");
        str("");
        u32(static_cast<std::uint32_t>(children.size()));
        for (std::uint64_t child : children) u64(child);
    }

    f32(5.0f); // duration
    f32(60.0f);
    u8(1);
    f32(1.0f);
    u32(0);       // track_count
    u32(0x21444E45); // end marker
    return out;
}

void test_error_paths()
{
    const auto valid = serialize(make_sample());

    // 空输入 / 截断。
    EXPECT_FAIL(std::vector<std::uint8_t>{}, "unexpected end of file");
    {
        auto bytes = valid;
        bytes.resize(valid.size() / 2);
        Project out;
        std::string error;
        assert(!deserialize(bytes, out, &error));
        assert(error.find("unexpected end of file") != std::string::npos
               || error.find("missing end marker") != std::string::npos);
    }

    // 坏魔术。
    {
        auto bytes = valid;
        bytes[0] = 'X';
        EXPECT_FAIL(bytes, "bad magic");
    }

    // guard 被文本模式污染。
    {
        auto bytes = valid;
        bytes[4] = 0x00;
        EXPECT_FAIL(bytes, "text-mode");
    }

    // 主版本不支持。
    {
        auto bytes = valid;
        bytes[8] = 2;
        EXPECT_FAIL(bytes, "unsupported project version 2.0");
    }

    // 次版本比当前新。
    {
        auto bytes = valid;
        bytes[9] = 1;
        EXPECT_FAIL(bytes, "newer than this build");
    }

    // 缺结束标记。
    {
        auto bytes = valid;
        bytes.resize(bytes.size() - 4);
        Project out;
        std::string error;
        assert(!deserialize(bytes, out, &error));
        assert(error.find("unexpected end of file") != std::string::npos
               || error.find("missing end marker") != std::string::npos);
    }

    // 自引用成环。
    EXPECT_FAIL(make_raw({{1, {1}}}), "cycle");

    // 两个节点互相引用。
    EXPECT_FAIL(make_raw({{1, {2}}, {2, {1}}}), "cycle");

    // 多重父。
    EXPECT_FAIL(make_raw({{1, {3}}, {2, {3}}, {3, {}}}), "multiple parents");

    // 引用不存在的子节点。
    EXPECT_FAIL(make_raw({{1, {9}}}), "unknown child");
}

void test_hierarchy_rebuild()
{
    // 合法层级：1 → 2 → 3。
    const auto bytes = make_raw({{1, {2}}, {2, {3}}, {3, {}}});
    Project out;
    std::string error;
    assert(deserialize(bytes, out, &error));
    assert(out.scene.size() == 3);
    assert(out.scene.find(2)->parent == 1);
    assert(out.scene.find(3)->parent == 2);
    assert(out.scene.find(1)->children.size() == 1);
}
} // namespace

int main()
{
    test_round_trip();
    test_deterministic();
    test_empty_round_trip();
    test_error_paths();
    test_hierarchy_rebuild();
    std::printf("test_anim_serialize: all passed\n");
    return 0;
}
