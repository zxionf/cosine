#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace xel::backend
{
struct AssetLocation
{
    std::string relative;
    std::filesystem::path path;
    bool found = false;
    std::vector<std::filesystem::path> searched;
};

// 按 COSINE_ASSET_DIR、可执行文件旁 assets/、构建期注入目录的顺序查找。
AssetLocation find_asset(const std::string& relative);

// 兼容旧调用：返回找到的路径；找不到时返回第一个候选路径。
std::filesystem::path asset_path(const std::string& relative);

// 找不到时抛 runtime_error，包含搜索目录与修复提示。
std::filesystem::path require_asset(const std::string& relative);

std::string format_asset_error(const AssetLocation& location);
} // namespace xel::backend
