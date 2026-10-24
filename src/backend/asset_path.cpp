#include "asset_path.hpp"

#include <cstdlib>
#include <vector>

#ifdef _WIN32
#  include <windows.h>
#else
#  include <unistd.h>
#endif

namespace
{
std::filesystem::path executable_dir()
{
#ifdef _WIN32
    char buffer[MAX_PATH]{};
    const DWORD length = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    if (length != 0 && length < MAX_PATH) {
        return std::filesystem::path(std::string(buffer, length)).parent_path();
    }
#else
    std::error_code ec;
    const auto path = std::filesystem::read_symlink("/proc/self/exe", ec);
    if (!ec) return path.parent_path();
#endif
    return {};
}

std::filesystem::path env_asset_dir()
{
    const char* value = std::getenv("COSINE_ASSET_DIR");
    return value ? std::filesystem::path(value) : std::filesystem::path{};
}
} // namespace

namespace xel::backend
{
std::filesystem::path asset_path(const std::string& relative)
{
    std::vector<std::filesystem::path> bases;

    if (const auto env = env_asset_dir(); !env.empty()) {
        bases.push_back(env);
    }

    if (const auto exe = executable_dir(); !exe.empty()) {
        bases.push_back(exe / "assets");
    }

#ifdef COSINE_ASSET_DIR
    bases.emplace_back(COSINE_ASSET_DIR);
#endif

    std::error_code ec;
    for (const auto& base : bases) {
        const auto candidate = base / relative;
        if (std::filesystem::exists(candidate, ec)) {
            return candidate;
        }
    }

    if (!bases.empty()) {
        return bases.front() / relative;
    }
    return relative;
}
} // namespace xel::backend
