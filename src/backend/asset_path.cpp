#include "asset_path.hpp"

#include <cstdlib>
#include <stdexcept>

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

std::vector<std::filesystem::path> asset_bases()
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
    return bases;
}
} // namespace

namespace xel::backend
{
AssetLocation find_asset(const std::string& relative)
{
    AssetLocation location;
    location.relative = relative;

    const auto bases = asset_bases();
    for (const auto& base : bases) {
        const auto candidate = base / relative;
        location.searched.push_back(candidate);

        std::error_code ec;
        if (std::filesystem::exists(candidate, ec) && !ec) {
            location.path = candidate;
            location.found = true;
            return location;
        }
    }

    if (!bases.empty()) {
        location.path = bases.front() / relative;
    } else {
        location.path = relative;
    }
    return location;
}

std::filesystem::path asset_path(const std::string& relative)
{
    return find_asset(relative).path;
}

std::filesystem::path require_asset(const std::string& relative)
{
    const auto location = find_asset(relative);
    if (location.found) return location.path;
    throw std::runtime_error(format_asset_error(location));
}

std::string format_asset_error(const AssetLocation& location)
{
    std::string message = "[Asset] file not found: " + location.relative + "\nsearched:";
    if (location.searched.empty()) {
        message += "\n  (no search paths; current working directory only)";
    } else {
        for (const auto& candidate : location.searched) {
            message += "\n  - " + candidate.string();
        }
    }
    message += "\nhint: set COSINE_ASSET_DIR, or place assets next to the executable.";
    return message;
}
} // namespace xel::backend
