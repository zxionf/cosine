#pragma once

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>

namespace xel::graphics
{
// ── 图元种类 ──────────────────────────────────────────────────────
//
// kind 决定 fragment 怎么解释这个顶点（一个 pipeline 内分支，不切批）：
//   Sprite  普通纹理/色块/文字：color * texture(uv)
//   Rect    解析圆角矩形：uv 是局部坐标，half/radius 来自顶点
//   Ring    解析圆角环：额外用 thickness 与外框求交
//   Atlas   MTSDF 图集图标：uv 采样图集，median 重建距离
enum class VertexKind : std::uint32_t
{
    Sprite = 0,
    Rect = 1,
    Ring = 2,
    Atlas = 3,
};

struct Vertex
{
    glm::vec2 pos;
    glm::vec4 color;
    glm::vec2 uv;
    // 解析形状参数：half = 未含圆角的外半尺寸；extra = (radius, thickness)。
    // kind == Sprite/Atlas 时全 0（Atlas 用 half.x 传 px_range）。
    glm::vec2 half{0.0f, 0.0f};
    glm::vec2 extra{0.0f, 0.0f};
    std::uint32_t kind = 0;

    static vk::VertexInputBindingDescription get_binding_description()
    {
        return {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = vk::VertexInputRate::eVertex
        };
    }
    static std::array<vk::VertexInputAttributeDescription, 6> get_attribute_description()
    {
        return {{
            {.location = 0, .binding = 0, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(Vertex, pos)},
            {.location = 1, .binding = 0, .format = vk::Format::eR32G32B32A32Sfloat, .offset = offsetof(Vertex, color)},
            {.location = 2, .binding = 0, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(Vertex, uv)},
            {.location = 3, .binding = 0, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(Vertex, half)},
            {.location = 4, .binding = 0, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(Vertex, extra)},
            {.location = 5, .binding = 0, .format = vk::Format::eR32Uint, .offset = offsetof(Vertex, kind)}
        }};
    }
};

// 顶点布局是 GPU 契约的一部分：52 字节 / align 4（glm 的 vec4 在未定义
// GLM_FORCE_ALIGNED_GENTYPES 时不强制 16 字节对齐，所以没有尾部 padding）。
// shader 侧的 attribute location 顺序必须与 get_attribute_description() 一致。
static_assert(sizeof(Vertex) == 52, "Vertex layout is part of the GPU contract");
static_assert(offsetof(Vertex, color) == 8, "color offset must match shader location 1");
static_assert(offsetof(Vertex, uv) == 24, "uv offset must match shader location 2");
static_assert(offsetof(Vertex, half) == 32, "half offset must match shader location 3");
static_assert(offsetof(Vertex, extra) == 40, "extra offset must match shader location 4");
static_assert(offsetof(Vertex, kind) == 48, "kind offset must match shader location 5");
} // namespace xel::graphics
