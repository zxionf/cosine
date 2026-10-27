#pragma once

#include "project.hpp"
#include "property.hpp"
#include "scene.hpp"
#include "timeline.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace xel::anim
{
// 可撤销编辑操作。
// 约定：CommandStack::push 会调用一次 apply；undo 调 revert，redo 再调 apply。
// 因此 apply/revert 必须可重复执行且成对（幂等）。
class Command
{
public:
    virtual ~Command() = default;
    virtual const char* name() const = 0;
    virtual void apply(Project& project) = 0;
    virtual void revert(Project& project) = 0;
};

// 撤销/重做栈。push 会清空 redo 分支；超过上限时丢弃最旧的 undo。
class CommandStack
{
public:
    explicit CommandStack(std::size_t limit = 256);

    void push(std::unique_ptr<Command> command, Project& project);
    bool undo(Project& project);
    bool redo(Project& project);

    bool can_undo() const { return !undo_.empty(); }
    bool can_redo() const { return !redo_.empty(); }
    std::size_t undo_count() const { return undo_.size(); }
    std::size_t redo_count() const { return redo_.size(); }
    std::size_t limit() const { return limit_; }
    void clear();

private:
    std::vector<std::unique_ptr<Command>> undo_;
    std::vector<std::unique_ptr<Command>> redo_;
    std::size_t limit_ = 256;
};

// 修改节点属性。before 在首次 apply 时自动记录。
class SetPropertyCommand final : public Command
{
public:
    SetPropertyCommand(scene::NodeId node, PropertyId property, PropertyValue after)
        : node_(node), property_(property), after_(std::move(after))
    {}

    const char* name() const override { return "set property"; }
    void apply(Project& project) override;
    void revert(Project& project) override;

private:
    scene::NodeId node_;
    PropertyId property_;
    PropertyValue after_;
    PropertyValue before_;
    bool has_before_ = false;
};

// 添加节点。node.id 为 0 时由 apply 分配；否则按指定 id 插入（redo 复用同一 id）。
// 子节点不随命令添加（由后续命令负责）。
class AddNodeCommand final : public Command
{
public:
    explicit AddNodeCommand(scene::Node node) : node_(std::move(node)) {}

    const char* name() const override { return "add node"; }
    void apply(Project& project) override;
    void revert(Project& project) override;

    scene::NodeId id() const { return node_.id; }

private:
    scene::Node node_;
};

// 删除节点及其整棵子树。apply 时保存子树快照（父在前），revert 时逐层恢复。
class RemoveNodeCommand final : public Command
{
public:
    explicit RemoveNodeCommand(scene::NodeId node) : node_(node) {}

    const char* name() const override { return "remove node"; }
    void apply(Project& project) override;
    void revert(Project& project) override;

private:
    scene::NodeId node_;
    scene::NodeId parent_ = scene::kInvalidNode;
    std::vector<scene::Node> snapshot_; // 父在前
    bool captured_ = false;
};

// 添加关键帧。key.time 用于定位与撤销。
class AddKeyCommand final : public Command
{
public:
    AddKeyCommand(scene::NodeId node, PropertyId property, Keyframe key)
        : node_(node), property_(property), key_(std::move(key))
    {}

    const char* name() const override { return "add keyframe"; }
    void apply(Project& project) override;
    void revert(Project& project) override;

private:
    scene::NodeId node_;
    PropertyId property_;
    Keyframe key_;
};

// 删除关键帧。apply 时记录被删关键帧的内容，revert 时按原值恢复。
class RemoveKeyCommand final : public Command
{
public:
    RemoveKeyCommand(scene::NodeId node, PropertyId property, float time)
        : node_(node), property_(property), time_(time)
    {}

    const char* name() const override { return "remove keyframe"; }
    void apply(Project& project) override;
    void revert(Project& project) override;

private:
    scene::NodeId node_;
    PropertyId property_;
    float time_ = 0.0f;
    Keyframe captured_{};
    bool has_captured_ = false;
};
} // namespace xel::anim
