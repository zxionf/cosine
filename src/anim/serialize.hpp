#pragma once

#include "project.hpp"
#include "scene.hpp"
#include "timeline.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace xel::anim
{
// 文件格式版本，见 docs/project_format.md。
inline constexpr std::uint8_t kFormatMajor = 1;
inline constexpr std::uint8_t kFormatMinor = 1;

// 序列化为二进制。输出是确定性的：节点按 id 升序、轨道按 (node, property) 升序、
// 关键帧按 time 升序。
std::vector<std::uint8_t> serialize(const Project& project);

// 从二进制还原。失败时返回 false，并在 error 非空时写入可读原因。
// 失败时 out 的内容未定义（调用方应丢弃）。
bool deserialize(std::span<const std::uint8_t> bytes, Project& out, std::string* error);

// 读写项目文件（M6.5）。错误同样通过 error 返回可读原因。
bool save_project_file(const std::string& path, const Project& project, std::string* error);
bool load_project_file(const std::string& path, Project& out, std::string* error);
} // namespace xel::anim
