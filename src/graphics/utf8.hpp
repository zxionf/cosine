#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace xel::graphics
{
inline char32_t utf8_decode(std::string_view text, std::size_t& index)
{
    if (index >= text.size()) return 0;

    const unsigned char c0 = static_cast<unsigned char>(text[index]);
    if (c0 < 0x80) {
        ++index;
        return c0;
    }

    int length = 0;
    char32_t codepoint = 0;
    if ((c0 & 0xE0) == 0xC0) {
        length = 2;
        codepoint = c0 & 0x1F;
    } else if ((c0 & 0xF0) == 0xE0) {
        length = 3;
        codepoint = c0 & 0x0F;
    } else if ((c0 & 0xF8) == 0xF0) {
        length = 4;
        codepoint = c0 & 0x07;
    } else {
        ++index;
        return 0xFFFD;
    }

    if (index + static_cast<std::size_t>(length) > text.size()) {
        ++index;
        return 0xFFFD;
    }

    for (int i = 1; i < length; ++i) {
        const unsigned char c = static_cast<unsigned char>(text[index + static_cast<std::size_t>(i)]);
        if ((c & 0xC0) != 0x80) {
            ++index;
            return 0xFFFD;
        }
        codepoint = (codepoint << 6) | (c & 0x3F);
    }

    index += static_cast<std::size_t>(length);
    return codepoint;
}

inline std::string utf8_encode(char32_t codepoint)
{
    std::string out;
    if (codepoint <= 0x7F) {
        out.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
        out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
        out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
        out.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
    return out;
}

inline std::size_t utf8_next(std::string_view text, std::size_t index)
{
    if (index >= text.size()) return text.size();
    ++index;
    while (index < text.size() && (static_cast<unsigned char>(text[index]) & 0xC0) == 0x80) ++index;
    return index;
}

inline std::size_t utf8_prev(std::string_view text, std::size_t index)
{
    if (index == 0) return 0;
    --index;
    while (index > 0 && (static_cast<unsigned char>(text[index]) & 0xC0) == 0x80) --index;
    return index;
}

inline std::size_t utf8_codepoint_count(std::string_view text)
{
    std::size_t count = 0;
    std::size_t index = 0;
    while (index < text.size()) {
        (void)utf8_decode(text, index);
        ++count;
    }
    return count;
}
} // namespace xel::graphics
