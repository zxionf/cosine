#pragma once

#include "property.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace xel::scene
{
// 节点 id 自增且不复用（序列化友好）；0 表示无效。
using NodeId = std::uint64_t;
inline constexpr NodeId kInvalidNode = 0;

enum class NodeType
{
    Group,
    Shape,
    Text,
    Image,
    Camera,
};

struct Transform
{
    glm::vec2 position{0.0f, 0.0f};
    float rotation = 0.0f; // 弧度
    glm::vec2 scale{1.0f, 1.0f};
};

// 单层节点：类型枚举 + 类型专属字段。
// 这样属性系统与序列化只面对一个结构体，不需要 dynamic_cast。
struct Node
{
    NodeId id = kInvalidNode;
    NodeId parent = kInvalidNode;
    NodeType type = NodeType::Group;
    std::string name;
    Transform transform;
    bool visible = true;
    std::vector<NodeId> children;

    // Shape / Image
    glm::vec2 size{0.0f, 0.0f};
    glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
    bool ellipse = false; // Shape：椭圆 vs 矩形

    // Text
    std::string text;
    float text_size = 16.0f;

    // Image
    // texture_path：资源路径，落盘用；texture_id：运行时图片槽索引 + 1，0 = 未绑定。
    // 拆开的原因：句柄随进程/上传顺序变化，路径才是稳定标识。
    std::string texture_path;
    std::uint32_t texture_id = 0;

    // Camera
    float zoom = 1.0f;

    bool has_children() const { return !children.empty(); }
};

class Scene
{
public:
    Scene();

    // 创建节点并挂到 root 下。
    NodeId create(NodeType type, std::string name = {});

    // 用指定 id 插入节点（序列化读回用）。id 为 0 或已存在时返回 kInvalidNode。
    // 不自动挂到 root，也不修正 parent：父子关系由调用方第二遍设置。
    // 会提升 next_id_，避免后续 create 复用已存在的 id。
    NodeId insert(Node node);

    // 清空所有节点（含默认 root）；root_ 重置为无效，next_id_ 归 1。
    void clear();
    // 指定 root 节点（必须存在）。序列化读回后用它恢复「新节点自动挂到 root」。
    bool set_root(NodeId id);

    Node* find(NodeId id);
    const Node* find(NodeId id) const;

    // 递归删除子树并从父节点摘除；root 不可删。
    bool destroy(NodeId id);

    // 移动节点：拒绝成环、拒绝无效 id、拒绝自身。
    bool set_parent(NodeId child, NodeId parent);

    NodeId root() const { return root_; }
    std::size_t size() const { return nodes_.size(); }
    const std::unordered_map<NodeId, Node>& nodes() const { return nodes_; }

    // 深度优先、先父后子；children 按存储顺序展开。
    void for_each(const std::function<void(const Node&)>& fn) const;

    // 属性读写：类型不匹配、属性未知或节点不存在时返回 nullopt / false。
    std::optional<anim::PropertyValue> get_property(NodeId id, anim::PropertyId prop) const;
    bool set_property(NodeId id, anim::PropertyId prop, const anim::PropertyValue& value);

private:
    // ancestor 是否是 node 自身或其祖先。
    bool is_descendant(NodeId ancestor, NodeId node) const;

    std::unordered_map<NodeId, Node> nodes_;
    NodeId root_ = kInvalidNode;
    NodeId next_id_ = 1;
};
} // namespace xel::scene
