#include "anim/scene.hpp"

#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

namespace
{
using namespace xel;

void test_root_and_create()
{
    scene::Scene s;
    assert(s.root() != scene::kInvalidNode);
    assert(s.size() == 1);

    const auto a = s.create(scene::NodeType::Shape, "a");
    const auto b = s.create(scene::NodeType::Text, "b");
    assert(a != b);
    assert(s.find(a) != nullptr);
    assert(s.find(a)->parent == s.root());
    assert(s.find(a)->name == "a");
    assert(s.find(b)->type == scene::NodeType::Text);

    const auto* root = s.find(s.root());
    assert(root->children.size() == 2);
    assert(root->children[0] == a);
    assert(root->children[1] == b);
}

void test_transform_and_type_fields()
{
    scene::Scene s;
    const auto id = s.create(scene::NodeType::Shape);

    auto* n = s.find(id);
    n->transform.position = {10.0f, 20.0f};
    n->transform.rotation = 1.5f;
    n->transform.scale = {2.0f, 3.0f};
    n->size = {100.0f, 50.0f};
    n->ellipse = true;
    n->color = {1.0f, 0.0f, 0.0f, 1.0f};

    const auto* read = s.find(id);
    assert(read->transform.position.x == 10.0f);
    assert(read->transform.rotation == 1.5f);
    assert(read->transform.scale.y == 3.0f);
    assert(read->size.x == 100.0f);
    assert(read->ellipse);
    assert(read->color.y == 0.0f);
}

void test_set_parent()
{
    scene::Scene s;
    const auto a = s.create(scene::NodeType::Group, "a");
    const auto b = s.create(scene::NodeType::Group, "b");
    const auto c = s.create(scene::NodeType::Shape, "c");

    assert(s.set_parent(c, a));
    assert(s.find(c)->parent == a);
    assert(s.find(a)->children.size() == 1);
    assert(s.find(s.root())->children.size() == 2); // a, b

    // 成环：a 是 c 的祖先，把 a 挂到 c 下必须失败。
    assert(!s.set_parent(a, c));
    // 自身
    assert(!s.set_parent(a, a));
    // 不存在的节点
    assert(!s.set_parent(c, 9999));
    assert(!s.set_parent(9999, a));
}

void test_destroy_recursive()
{
    scene::Scene s;
    const auto a = s.create(scene::NodeType::Group, "a");
    const auto b = s.create(scene::NodeType::Group, "b");
    const auto c = s.create(scene::NodeType::Shape, "c");
    s.set_parent(b, a);
    s.set_parent(c, b);

    assert(s.size() == 4);
    assert(s.destroy(a));
    assert(s.size() == 1); // 只剩 root
    assert(s.find(a) == nullptr);
    assert(s.find(b) == nullptr);
    assert(s.find(c) == nullptr);
    assert(s.find(s.root())->children.empty());

    // root 不可删；重复删除返回 false。
    assert(!s.destroy(s.root()));
    assert(!s.destroy(a));
}

void test_ids_not_reused()
{
    scene::Scene s;
    const auto a = s.create(scene::NodeType::Group);
    assert(s.destroy(a));
    const auto b = s.create(scene::NodeType::Group);
    assert(b > a);
    assert(b != a);
}

void test_for_each_order()
{
    scene::Scene s;
    const auto a = s.create(scene::NodeType::Group, "a");
    const auto b = s.create(scene::NodeType::Group, "b");
    const auto a1 = s.create(scene::NodeType::Shape, "a1");
    const auto a2 = s.create(scene::NodeType::Shape, "a2");
    s.set_parent(a1, a);
    s.set_parent(a2, a);

    std::vector<std::string> order;
    s.for_each([&](const scene::Node& n) { order.push_back(n.name); });

    assert(order.size() == 5);
    assert(order[0] == "root");
    assert(order[1] == "a");
    assert(order[2] == "a1");
    assert(order[3] == "a2");
    assert(order[4] == "b");
}

void test_hidden_still_traversed()
{
    scene::Scene s;
    const auto a = s.create(scene::NodeType::Shape, "a");
    s.find(a)->visible = false;

    std::size_t count = 0;
    s.for_each([&](const scene::Node&) { ++count; });
    assert(count == 2); // root + a
}
} // namespace

int main()
{
    test_root_and_create();
    test_transform_and_type_fields();
    test_set_parent();
    test_destroy_recursive();
    test_ids_not_reused();
    test_for_each_order();
    test_hidden_still_traversed();
    std::printf("test_anim_scene: all passed\n");
    return 0;
}
