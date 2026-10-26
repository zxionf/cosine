#include "anim/scene.hpp"
#include "graphics/drawable.hpp"
#include "ui/scene_view.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>

namespace
{
using namespace xel;

std::size_t vertex_count(const graphics::DrawList& list)
{
    std::size_t total = 0;
    for (const auto& batch : list.batches()) total += batch.vertices.size();
    return total;
}

void test_draws_shape()
{
    scene::Scene s;
    const auto n = s.create(scene::NodeType::Shape, "box");
    s.find(n)->size = {40.0f, 20.0f};
    s.find(n)->color = {1.0f, 0.0f, 0.0f, 1.0f};

    ui::SceneView view(&s);
    view.arrange({{0.0f, 0.0f}, {200.0f, 200.0f}});

    graphics::DrawList list;
    list.set_full_clip({0.0f, 0.0f, 800.0f, 600.0f});
    view.append_to(list);
    assert(vertex_count(list) > 0);
}

void test_hidden_subtree_skipped()
{
    scene::Scene s;
    const auto parent = s.create(scene::NodeType::Group, "p");
    const auto child = s.create(scene::NodeType::Shape, "c");
    s.set_parent(child, parent);
    s.find(child)->size = {10.0f, 10.0f};
    s.find(parent)->visible = false;

    ui::SceneView view(&s);
    view.arrange({{0.0f, 0.0f}, {200.0f, 200.0f}});

    graphics::DrawList list;
    list.set_full_clip({0.0f, 0.0f, 800.0f, 600.0f});
    view.append_to(list);
    assert(vertex_count(list) == 0);
}

void test_parent_transform_accumulates()
{
    scene::Scene s;
    const auto parent = s.create(scene::NodeType::Group, "p");
    const auto child = s.create(scene::NodeType::Shape, "c");
    s.set_parent(child, parent);
    s.find(parent)->transform.position = {50.0f, 30.0f};
    s.find(child)->transform.position = {10.0f, 5.0f};
    s.find(child)->size = {20.0f, 20.0f};

    ui::SceneView view(&s);
    view.arrange({{0.0f, 0.0f}, {200.0f, 200.0f}});

    graphics::DrawList list;
    list.set_full_clip({0.0f, 0.0f, 800.0f, 600.0f});
    view.append_to(list);

    assert(!list.batches().empty());
    const auto& batch = list.batches().front();
    assert(!batch.vertices.empty());

    // 父 (50,30) + 子 (10,5) = (60,35)。
    assert(std::fabs(batch.vertices.front().pos.x - 60.0f) < 1e-3f);
    assert(std::fabs(batch.vertices.front().pos.y - 35.0f) < 1e-3f);
}

void test_scale_accumulates()
{
    scene::Scene s;
    const auto parent = s.create(scene::NodeType::Group, "p");
    const auto child = s.create(scene::NodeType::Shape, "c");
    s.set_parent(child, parent);
    s.find(parent)->transform.scale = {2.0f, 2.0f};
    s.find(child)->size = {10.0f, 10.0f};

    ui::SceneView view(&s);
    view.arrange({{0.0f, 0.0f}, {200.0f, 200.0f}});

    graphics::DrawList list;
    list.set_full_clip({0.0f, 0.0f, 800.0f, 600.0f});
    view.append_to(list);

    const auto& batch = list.batches().front();
    // 矩形宽 = 10 * 2 = 20：第 2 个顶点 x = 20。
    assert(std::fabs(batch.vertices[1].pos.x - 20.0f) < 1e-3f);
}

void test_empty_scene()
{
    scene::Scene s;
    ui::SceneView view(&s);
    view.arrange({{0.0f, 0.0f}, {100.0f, 100.0f}});

    graphics::DrawList list;
    list.set_full_clip({0.0f, 0.0f, 800.0f, 600.0f});
    view.append_to(list);
    assert(vertex_count(list) == 0);
}
} // namespace

int main()
{
    test_draws_shape();
    test_hidden_subtree_skipped();
    test_parent_transform_accumulates();
    test_scale_accumulates();
    test_empty_scene();
    std::printf("test_ui_scene_view: all passed\n");
    return 0;
}
