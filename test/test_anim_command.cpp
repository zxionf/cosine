#include "anim/command.hpp"

#include "anim/easing.hpp"
#include "anim/project.hpp"
#include "anim/property.hpp"
#include "anim/scene.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <variant>

namespace
{
namespace ns = xel::scene;
namespace prop = xel::anim::prop;
using xel::anim::AddKeyCommand;
using xel::anim::AddNodeCommand;
using xel::anim::Command;
using xel::anim::CommandStack;
using xel::anim::CompoundCommand;
using xel::anim::Easing;
using xel::anim::Keyframe;
using xel::anim::MoveKeyCommand;
using xel::anim::Project;
using xel::anim::RemoveKeyCommand;
using xel::anim::RemoveNodeCommand;
using xel::anim::SetNodeColorCommand;
using xel::anim::SetNodeFieldCommand;
using xel::anim::SetPropertyCommand;
using xel::anim::Track;

constexpr float kEps = 1.0e-6f;

glm::vec2 position_of(const Project& p, ns::NodeId node)
{
    const auto value = p.scene.get_property(node, prop::Position);
    assert(value.has_value());
    return std::get<glm::vec2>(*value);
}

void test_set_property_undo_redo()
{
    Project p;
    const auto node = p.scene.create(ns::NodeType::Shape, "s");
    const glm::vec2 before = position_of(p, node);

    CommandStack stack;
    stack.push(std::make_unique<SetPropertyCommand>(node, prop::Position, glm::vec2{5.0f, 6.0f}), p);
    assert(std::fabs(position_of(p, node).x - 5.0f) < kEps);
    assert(stack.can_undo() && !stack.can_redo());

    assert(stack.undo(p));
    assert(std::fabs(position_of(p, node).x - before.x) < kEps);
    assert(!stack.can_undo() && stack.can_redo());

    assert(stack.redo(p));
    assert(std::fabs(position_of(p, node).x - 5.0f) < kEps);
}

// 显式 before/after 的字段修改（拖动数值框的撤销单元）。
void test_set_node_field_command()
{
    Project p;
    const auto node = p.scene.create(ns::NodeType::Shape, "s");
    p.scene.find(node)->transform.position = {10.0f, 20.0f};

    CommandStack stack;
    auto command = std::make_unique<SetNodeFieldCommand>(
        node, "Pos X", [](ns::Node& n, float v) { n.transform.position.x = v; }, 10.0f, 50.0f);
    stack.push(std::move(command), p);
    assert(std::fabs(p.scene.find(node)->transform.position.x - 50.0f) < kEps);
    // y 不受影响。
    assert(std::fabs(p.scene.find(node)->transform.position.y - 20.0f) < kEps);

    assert(stack.undo(p));
    assert(std::fabs(p.scene.find(node)->transform.position.x - 10.0f) < kEps);

    assert(stack.redo(p));
    assert(std::fabs(p.scene.find(node)->transform.position.x - 50.0f) < kEps);
}

// 颜色修改（vec4）单独一条命令。
void test_set_node_color_command()
{
    Project p;
    const auto node = p.scene.create(ns::NodeType::Shape, "s");
    p.scene.find(node)->color = {0.1f, 0.2f, 0.3f, 1.0f};

    CommandStack stack;
    stack.push(std::make_unique<SetNodeColorCommand>(node, glm::vec4{0.1f, 0.2f, 0.3f, 1.0f},
                                                     glm::vec4{0.9f, 0.8f, 0.7f, 1.0f}),
               p);
    assert(std::fabs(p.scene.find(node)->color.r - 0.9f) < kEps);

    assert(stack.undo(p));
    assert(std::fabs(p.scene.find(node)->color.r - 0.1f) < kEps);

    assert(stack.redo(p));
    assert(std::fabs(p.scene.find(node)->color.r - 0.9f) < kEps);
}

void test_add_node_undo_redo()
{
    Project p;

    ns::Node node;
    node.type = ns::NodeType::Shape;
    node.name = "added";
    node.transform.position = {1.0f, 2.0f};
    node.color = {0.1f, 0.2f, 0.3f, 1.0f};

    CommandStack stack;
    auto command = std::make_unique<AddNodeCommand>(node);
    AddNodeCommand* raw = command.get();
    stack.push(std::move(command), p);

    const auto id = raw->id();
    assert(id == 2); // root 占 id=1
    assert(p.scene.size() == 2); // root + added
    const auto* created = p.scene.find(id);
    assert(created && created->name == "added");
    assert(std::fabs(created->transform.position.y - 2.0f) < kEps);
    assert(std::fabs(created->color.z - 0.3f) < kEps);

    assert(stack.undo(p));
    assert(p.scene.size() == 1); // 只剩 root

    // redo 复用同一 id。
    assert(stack.redo(p));
    assert(p.scene.size() == 2);
    assert(p.scene.find(id) != nullptr);
    assert(p.scene.find(id)->name == "added");
}

void test_add_node_with_explicit_id()
{
    Project p;
    ns::Node node;
    node.id = 42;
    node.type = ns::NodeType::Group;
    node.name = "fixed";

    CommandStack stack;
    stack.push(std::make_unique<AddNodeCommand>(node), p);
    assert(p.scene.find(42) != nullptr);
    assert(p.scene.find(42)->name == "fixed");

    assert(stack.undo(p));
    assert(p.scene.find(42) == nullptr);

    assert(stack.redo(p));
    assert(p.scene.find(42) != nullptr);
}

void test_remove_node_subtree()
{
    Project p;
    const auto group = p.scene.create(ns::NodeType::Group, "group");
    const auto child = p.scene.create(ns::NodeType::Shape, "child");
    p.scene.set_parent(child, group);
    const auto grandchild = p.scene.create(ns::NodeType::Text, "text");
    p.scene.set_parent(grandchild, child);
    p.scene.find(grandchild)->text = "hi";

    CommandStack stack;
    stack.push(std::make_unique<RemoveNodeCommand>(group), p);
    assert(p.scene.size() == 1); // 只剩 root

    assert(stack.undo(p));
    assert(p.scene.size() == 4); // root + group + child + grandchild
    const auto* g = p.scene.find(group);
    const auto* c = p.scene.find(child);
    const auto* t = p.scene.find(grandchild);
    assert(g && c && t);
    assert(g->children.size() == 1 && g->children[0] == child);
    assert(c->children.size() == 1 && c->children[0] == grandchild);
    assert(c->parent == group);
    assert(t->parent == child);
    assert(t->text == "hi");

    // undo 后再次 redo，仍然干净。
    assert(stack.redo(p));
    assert(p.scene.size() == 1);
    assert(stack.undo(p));
    assert(p.scene.size() == 4);
}

void test_key_commands()
{
    Project p;
    const auto node = p.scene.create(ns::NodeType::Shape, "s");

    CommandStack stack;
    Keyframe key{1.0f, glm::vec2{3.0f, 4.0f}, Easing::EaseIn};
    stack.push(std::make_unique<AddKeyCommand>(node, prop::Position, key), p);

    const Track* track = p.timeline.find_track(node, prop::Position);
    assert(track != nullptr && track->keys().size() == 1);

    // 撤销后空轨道被移除。
    assert(stack.undo(p));
    assert(p.timeline.find_track(node, prop::Position) == nullptr);

    assert(stack.redo(p));
    track = p.timeline.find_track(node, prop::Position);
    assert(track != nullptr && track->keys().size() == 1);
    assert(track->keys()[0].easing == Easing::EaseIn);

    // 删除关键帧后可用原值恢复。
    CommandStack remove_stack;
    remove_stack.push(std::make_unique<RemoveKeyCommand>(node, prop::Position, 1.0f), p);
    assert(p.timeline.find_track(node, prop::Position) == nullptr);

    assert(remove_stack.undo(p));
    const Track* restored = p.timeline.find_track(node, prop::Position);
    assert(restored != nullptr && restored->keys().size() == 1);
    assert(std::fabs(std::get<glm::vec2>(restored->keys()[0].value).x - 3.0f) < kEps);
    assert(restored->keys()[0].easing == Easing::EaseIn);
}

void test_push_clears_redo()
{
    Project p;
    const auto node = p.scene.create(ns::NodeType::Shape, "s");

    CommandStack stack;
    stack.push(std::make_unique<SetPropertyCommand>(node, prop::Position, glm::vec2{1.0f, 0.0f}), p);
    assert(stack.undo(p));
    assert(stack.can_redo());

    stack.push(std::make_unique<SetPropertyCommand>(node, prop::Position, glm::vec2{2.0f, 0.0f}), p);
    assert(!stack.can_redo());
    assert(p.scene.get_property(node, prop::Position));
    assert(std::fabs(position_of(p, node).x - 2.0f) < kEps);
}

void test_limit_drops_oldest()
{
    Project p;
    const auto node = p.scene.create(ns::NodeType::Shape, "s");

    CommandStack stack(2);
    for (int i = 0; i < 5; ++i) {
        stack.push(std::make_unique<SetPropertyCommand>(node, prop::Position,
                                                        glm::vec2{static_cast<float>(i), 0.0f}),
                   p);
    }
    assert(stack.undo_count() == 2);
    assert(stack.limit() == 2);
}

void test_empty_stack()
{
    Project p;
    CommandStack stack;
    assert(!stack.undo(p));
    assert(!stack.redo(p));
    assert(!stack.can_undo() && !stack.can_redo());
    stack.clear();
    assert(stack.undo_count() == 0);
}
// 复合命令：一次 apply 全部生效，一次 undo 全部回滚（逆序）。
void test_compound_command()
{
    Project p;
    const auto node = p.scene.create(ns::NodeType::Shape, "n");
    p.timeline.add_track(node, prop::Position).add_key({1.0f, glm::vec2{0.0f, 0.0f}, Easing::Linear});
    p.timeline.add_track(node, prop::Scale).add_key({1.0f, glm::vec2{1.0f, 1.0f}, Easing::Linear});

    auto key_time = [&p, node](xel::anim::PropertyId property) {
        const Track* track = p.timeline.find_track(node, property);
        assert(track != nullptr && !track->keys().empty());
        return track->keys().front().time;
    };

    CommandStack stack;
    std::vector<std::unique_ptr<Command>> children;
    children.push_back(std::make_unique<MoveKeyCommand>(node, prop::Position, 1.0f, 2.0f));
    children.push_back(std::make_unique<MoveKeyCommand>(node, prop::Scale, 1.0f, 3.0f));
    stack.push(std::make_unique<CompoundCommand>(std::move(children)), p);

    assert(stack.undo_count() == 1);
    assert(std::fabs(key_time(prop::Position) - 2.0f) < kEps);
    assert(std::fabs(key_time(prop::Scale) - 3.0f) < kEps);

    assert(stack.undo(p));
    assert(stack.undo_count() == 0);
    assert(std::fabs(key_time(prop::Position) - 1.0f) < kEps);
    assert(std::fabs(key_time(prop::Scale) - 1.0f) < kEps);

    assert(stack.redo(p));
    assert(std::fabs(key_time(prop::Position) - 2.0f) < kEps);
    assert(std::fabs(key_time(prop::Scale) - 3.0f) < kEps);
}
} // namespace

int main()
{
    test_set_property_undo_redo();
    test_set_node_field_command();
    test_set_node_color_command();
    test_add_node_undo_redo();
    test_add_node_with_explicit_id();
    test_remove_node_subtree();
    test_key_commands();
    test_push_clears_redo();
    test_limit_drops_oldest();
    test_empty_stack();
    test_compound_command();
    std::printf("test_anim_command: all passed\n");
    return 0;
}
