#pragma once

#include <filesystem>
#include <string>

namespace xel::backend
{
// 解析资源路径：依次尝试 COSINE_ASSET_DIR 环境变量、可执行文件旁的 assets/、构建期注入的目录。
std::filesystem::path asset_path(const std::string& relative);
} // namespace xel::backend
