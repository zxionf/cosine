#include "command.hpp"

#include <utility>

namespace xel::anim
{
CommandStack::CommandStack(std::size_t limit) : limit_(limit == 0 ? 1 : limit) {}

void CommandStack::push(std::unique_ptr<Command> command, Project& project)
{
    if (!command) return;
    command->apply(project);
    undo_.push_back(std::move(command));
    redo_.clear();
    while (undo_.size() > limit_) undo_.erase(undo_.begin());
}

bool CommandStack::undo(Project& project)
{
    if (undo_.empty()) return false;
    std::unique_ptr<Command> command = std::move(undo_.back());
    undo_.pop_back();
    command->revert(project);
    redo_.push_back(std::move(command));
    return true;
}

bool CommandStack::redo(Project& project)
{
    if (redo_.empty()) return false;
    std::unique_ptr<Command> command = std::move(redo_.back());
    redo_.pop_back();
    command->apply(project);
    undo_.push_back(std::move(command));
    return true;
}

void CommandStack::clear()
{
    undo_.clear();
    redo_.clear();
}

void SetPropertyCommand::apply(Project& project)
{
    if (!has_before_) {
        if (auto current = project.scene.get_property(node_, property_)) {
            before_ = std::move(*current);
        } else {
            // 属性此前不存在：撤销时写回 after 等价于「无变化」，
            // 因为 set_property 对不支持的类型会直接返回 false。
            before_ = after_;
        }
        has_before_ = true;
    }
    project.scene.set_property(node_, property_, after_);
}

void SetPropertyCommand::revert(Project& project)
{
    if (has_before_) project.scene.set_property(node_, property_, before_);
}

void AddNodeCommand::apply(Project& project)
{
    scene::NodeId id = node_.id;
    if (id == scene::kInvalidNode) {
        id = project.scene.create(node_.type, node_.name);
        if (id == scene::kInvalidNode) return;
        node_.id = id;
    } else {
        scene::Node copy = node_;
        copy.children.clear(); // 子节点由后续命令添加
        if (project.scene.insert(std::move(copy)) == scene::kInvalidNode) return;
    }

    // 用快照覆盖 create/insert 产生的默认字段，但保留 id、parent 与已有子节点。
    if (auto* target = project.scene.find(id)) {
        const scene::NodeId keep_id = target->id;
        const scene::NodeId keep_parent = target->parent;
        const std::vector<scene::NodeId> keep_children = target->children;
        *target = node_;
        target->id = keep_id;
        target->parent = keep_parent;
        target->children = keep_children;
    }

    if (node_.parent != scene::kInvalidNode) {
        project.scene.set_parent(id, node_.parent);
    }
}

void AddNodeCommand::revert(Project& project)
{
    if (node_.id != scene::kInvalidNode) project.scene.destroy(node_.id);
}

void RemoveNodeCommand::apply(Project& project)
{
    if (!captured_) {
        const scene::Node* node = project.scene.find(node_);
        if (node == nullptr) return;

        parent_ = node->parent;
        // 深度优先、父在前收集子树。
        std::vector<scene::NodeId> stack{node_};
        while (!stack.empty()) {
            const scene::NodeId current = stack.back();
            stack.pop_back();
            const scene::Node* found = project.scene.find(current);
            if (found == nullptr) continue;
            snapshot_.push_back(*found);
            for (auto it = found->children.rbegin(); it != found->children.rend(); ++it) {
                stack.push_back(*it);
            }
        }
        captured_ = true;
    }
    project.scene.destroy(node_);
}

void RemoveNodeCommand::revert(Project& project)
{
    if (!captured_) return;

    // 两遍：先按 id 插入（子节点列表先清空），再恢复层级。
    for (const scene::Node& node : snapshot_) {
        scene::Node copy = node;
        copy.children.clear();
        project.scene.insert(std::move(copy));
    }
    for (const scene::Node& node : snapshot_) {
        if (auto* target = project.scene.find(node.id)) target->children = node.children;
    }
    for (const scene::Node& node : snapshot_) {
        for (const scene::NodeId child : node.children) {
            if (auto* target = project.scene.find(child)) target->parent = node.id;
        }
    }

    if (parent_ != scene::kInvalidNode) project.scene.set_parent(node_, parent_);
}

void AddKeyCommand::apply(Project& project)
{
    project.timeline.ensure_track(node_, property_).add_key(key_);
}

void AddKeyCommand::revert(Project& project)
{
    Track* track = project.timeline.find_track(node_, property_);
    if (track == nullptr) return;
    track->remove_key(key_.time);
    if (track->keys().empty()) project.timeline.remove_track(node_, property_);
}

void RemoveKeyCommand::apply(Project& project)
{
    Track* track = project.timeline.find_track(node_, property_);
    if (track == nullptr) return;

    if (!has_captured_) {
        for (const Keyframe& key : track->keys()) {
            if (key.time == time_) {
                captured_ = key;
                has_captured_ = true;
                break;
            }
        }
    }

    track->remove_key(time_);
    if (track->keys().empty()) project.timeline.remove_track(node_, property_);
}

void RemoveKeyCommand::revert(Project& project)
{
    if (!has_captured_) return;
    project.timeline.ensure_track(node_, property_).add_key(captured_);
}
} // namespace xel::anim
