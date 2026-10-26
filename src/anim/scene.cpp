#include "scene.hpp"

#include <algorithm>
#include <utility>

namespace xel::scene
{
Scene::Scene()
{
    root_ = next_id_++;
    Node root;
    root.id = root_;
    root.type = NodeType::Group;
    root.name = "root";
    nodes_.emplace(root_, std::move(root));
}

NodeId Scene::create(NodeType type, std::string name)
{
    const NodeId id = next_id_++;
    Node node;
    node.id = id;
    node.type = type;
    node.name = std::move(name);
    nodes_.emplace(id, std::move(node));

    if (root_ != kInvalidNode) {
        set_parent(id, root_);
    }
    return id;
}

Node* Scene::find(NodeId id)
{
    const auto it = nodes_.find(id);
    return it == nodes_.end() ? nullptr : &it->second;
}

const Node* Scene::find(NodeId id) const
{
    const auto it = nodes_.find(id);
    return it == nodes_.end() ? nullptr : &it->second;
}

bool Scene::is_descendant(NodeId ancestor, NodeId node) const
{
    NodeId cur = node;
    while (cur != kInvalidNode) {
        if (cur == ancestor) return true;
        const auto it = nodes_.find(cur);
        if (it == nodes_.end()) break;
        cur = it->second.parent;
    }
    return false;
}

bool Scene::set_parent(NodeId child, NodeId parent)
{
    if (child == kInvalidNode || child == parent) return false;

    auto c = nodes_.find(child);
    auto p = nodes_.find(parent);
    if (c == nodes_.end() || p == nodes_.end()) return false;

    // parent 若是 child 的后代，会成环。
    if (is_descendant(child, parent)) return false;

    // 从旧父摘除。
    if (auto* old_parent = find(c->second.parent)) {
        auto& list = old_parent->children;
        list.erase(std::remove(list.begin(), list.end(), child), list.end());
    }

    c->second.parent = parent;
    p->second.children.push_back(child);
    return true;
}

bool Scene::destroy(NodeId id)
{
    if (id == kInvalidNode || id == root_) return false;

    const auto it = nodes_.find(id);
    if (it == nodes_.end()) return false;

    // 先从父节点摘除。
    if (auto* parent = find(it->second.parent)) {
        auto& list = parent->children;
        list.erase(std::remove(list.begin(), list.end(), id), list.end());
    }

    // 迭代删除整棵子树，避免边遍历边删。
    std::vector<NodeId> stack{id};
    while (!stack.empty()) {
        const NodeId cur = stack.back();
        stack.pop_back();
        const auto node = nodes_.find(cur);
        if (node == nodes_.end()) continue;
        for (NodeId child : node->second.children) {
            stack.push_back(child);
        }
        nodes_.erase(node);
    }
    return true;
}

void Scene::for_each(const std::function<void(const Node&)>& fn) const
{
    if (root_ == kInvalidNode) return;

    std::vector<NodeId> stack{root_};
    while (!stack.empty()) {
        const NodeId cur = stack.back();
        stack.pop_back();

        const auto it = nodes_.find(cur);
        if (it == nodes_.end()) continue;

        fn(it->second);

        // 逆序压栈，弹出时才是 children 的正序。
        const auto& kids = it->second.children;
        for (auto rit = kids.rbegin(); rit != kids.rend(); ++rit) {
            stack.push_back(*rit);
        }
    }
}
} // namespace xel::scene
