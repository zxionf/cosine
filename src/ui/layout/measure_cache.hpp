#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <string_view>

namespace xel::ui
{
// 文本键的哈希：键里不放 std::string。
//
// 为什么要哈希而不是存文本：MeasureKey 是**每个控件**都内嵌的成员（MeasureCache 里一份），
// 塞一个 std::string 会让所有控件多付 32 B（sizeof(Widget) 384 → 456）。
// 缓存只需要“判等”，不需要原文，所以存 FNV-1a 64 位哈希 + 长度即可；
// 碰撞概率 ~2^-64，且退化的后果只是少命中一次（重新测量），不影响正确性。
inline std::uint64_t measure_hash_text(std::string_view text)
{
    std::uint64_t hash = 0xcbf29ce484222325ull; // FNV-1a offset basis
    for (const char c : text) {
        hash ^= static_cast<std::uint64_t>(static_cast<unsigned char>(c));
        hash *= 0x100000001b3ull; // FNV prime
    }
    hash ^= static_cast<std::uint64_t>(text.size()) * 0x9e3779b97f4a7c15ull;
    return hash;
}

// 测量缓存键：一套缓存，键可扩展。
//
// 布局级输入（constraints + 轮次）所有控件都有；后面的派生输入默认取
// “不参与比较”的值，不用它的控件不需要填。
struct MeasureKey
{
    // 布局级输入
    glm::vec2 constraints{0.0f, 0.0f};
    std::uint64_t epoch = 0;

    // 派生输入（默认值 = 不参与比较）
    std::uint64_t text_hash = 0;     // measure_hash_text(text)；空文本 = 0
    float font_size = 0.0f;
    float wrap_width = -1.0f;
    float line_spacing = 0.0f;
    int max_lines = 0;
    bool wrap = false;
    const void* resource = nullptr;  // FontAtlas 等外部资源指针

    bool same_layout(const MeasureKey& other) const
    {
        return constraints == other.constraints && epoch == other.epoch;
    }

    bool operator==(const MeasureKey& other) const
    {
        return same_layout(other)
            && text_hash == other.text_hash
            && font_size == other.font_size
            && wrap_width == other.wrap_width
            && line_spacing == other.line_spacing
            && max_lines == other.max_lines
            && wrap == other.wrap
            && resource == other.resource;
    }
    bool operator!=(const MeasureKey& other) const { return !(*this == other); }
};

// 单个控件的测量缓存（唯一一份）。
struct MeasureCache
{
    MeasureKey key{};
    glm::vec2 result{0.0f, 0.0f};
    bool valid = false;

    bool hit(const MeasureKey& probe) const { return valid && key == probe; }
    void store(const MeasureKey& probe, glm::vec2 value)
    {
        key = probe;
        result = value;
        valid = true;
    }
    void invalidate() { valid = false; }
};
} // namespace xel::ui
