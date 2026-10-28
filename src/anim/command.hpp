#pragma once

#include "project.hpp"
#include "property.hpp"
#include "scene.hpp"
#include "timeline.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
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

// 显式记录 before/after 的属性修改。用于「值已经改完、只需补记一条命令」的场景
// （如拖动数值框：拖动开始时记 before，结束时 push）。
class SetPropertyValueCommand final : public Command
{
public:
    SetPropertyValueCommand(scene::NodeId node, PropertyId property, PropertyValue before,
                            PropertyValue after)
        : node_(node), property_(property), before_(std::move(before)), after_(std::move(after))
    {}

    const char* name() const override { return "set property value"; }
    void apply(Project& project) override { project.scene.set_property(node_, property_, after_); }
    void revert(Project& project) override { project.scene.set_property(node_, property_, before_); }

private:
    scene::NodeId node_;
    PropertyId property_;
    PropertyValue before_;
    PropertyValue after_;
};

// 修改节点的任意 float 字段（位置分量、缩放、尺寸、颜色分量……）。
// write 由调用方提供，不关心具体属性 id；用于「拖动数值框」这类已改完值再补记的场景。
class SetNodeFieldCommand final : public Command
{
public:
    using Writer = std::function<void(scene::Node&, float)>;

    SetNodeFieldCommand(scene::NodeId node, std::string label, Writer write, float before,
                        float after)
        : node_(node), label_(std::move(label)), write_(std::move(write)), before_(before),
          after_(after)
    {}

    const char* name() const override { return label_.c_str(); }

    void apply(Project& project) override
    {
        if (auto* node = project.scene.find(node_)) write_(*node, after_);
    }

    void revert(Project& project) override
    {
        if (auto* node = project.scene.find(node_)) write_(*node, before_);
    }

private:
    scene::NodeId node_;
    std::string label_;
    Writer write_;
    float before_ = 0.0f;
    float after_ = 0.0f;
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

// 修改节点颜色（vec4）。字段命令只能处理 float，颜色单独一条。
class SetNodeColorCommand final : public Command
{
public:
    SetNodeColorCommand(scene::NodeId node, glm::vec4 before, glm::vec4 after)
        : node_(node), before_(before), after_(after)
    {}

    const char* name() const override { return "set node color"; }
    void apply(Project& project) override;
    void revert(Project& project) override;

private:
    scene::NodeId node_;
    glm::vec4 before_{};
    glm::vec4 after_{};
};

// 移动关键帧。apply 在 push 时可能找不到 from（已在 to），幂等返回 false。
class MoveKeyCommand final : public Command
{
public:
    MoveKeyCommand(scene::NodeId node, PropertyId property, float from, float to)
        : node_(node), property_(property), from_(from), to_(to)
    {}

    const char* name() const override { return "move keyframe"; }
    void apply(Project& project) override;
    void revert(Project& project) override;

private:
    scene::NodeId node_;
    PropertyId property_;
    float from_ = 0.0f;
    float to_ = 0.0f;
};

// 修改关键帧插值。
class SetKeyEasingCommand final : public Command
{
public:
    SetKeyEasingCommand(scene::NodeId node, PropertyId property, float time, Easing from,
                        Easing to)
        : node_(node), property_(property), time_(time), from_(from), to_(to)
    {}

    const char* name() const override { return "set keyframe easing"; }
    void apply(Project& project) override;
    void revert(Project& project) override;

private:
    scene::NodeId node_;
    PropertyId property_;
    float time_ = 0.0f;
    Easing from_ = Easing::Linear;
    Easing to_ = Easing::Linear;
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
// 把多个命令当成一个整体：apply 正序、revert 逆序。
// 多选拖动多关键帧时用它，撤销一次就全部回滚。
class CompoundCommand final : public Command
{
public:
    explicit CompoundCommand(std::vector<std::unique_ptr<Command>> children)
        : children_(std::move(children))
    {}

    const char* name() const override { return "compound"; }
    void apply(Project& project) override;
    void revert(Project& project) override;
    std::size_t size() const { return children_.size(); }

private:
    std::vector<std::unique_ptr<Command>> children_;
};
} // namespace xel::anim
