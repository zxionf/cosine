#pragma once

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace xel::ui
{
// 菜单项：普通项、分隔线（label 为空）、快捷键提示。
// 递归结构天然支持任意层子菜单；子菜单内容按值持有（菜单项数量少，换来地址稳定）。
struct MenuItem
{
    std::string label;
    std::string shortcut;
    bool separator = false;
    bool enabled = true;
    std::function<void()> action;
    std::vector<MenuItem> children;

    static MenuItem separator_item()
    {
        MenuItem item;
        item.separator = true;
        return item;
    }

    static MenuItem make(std::string label, std::function<void()> action,
                         std::string shortcut = {}, bool enabled = true)
    {
        MenuItem item;
        item.label = std::move(label);
        item.shortcut = std::move(shortcut);
        item.action = std::move(action);
        item.enabled = enabled;
        return item;
    }

    // 子菜单项：children 非空时显示右向箭头，悬停/Enter 展开。
    static MenuItem submenu(std::string label, std::vector<MenuItem> children, bool enabled = true)
    {
        MenuItem item;
        item.label = std::move(label);
        item.children = std::move(children);
        item.enabled = enabled;
        return item;
    }

    bool has_submenu() const { return !children.empty(); }
};
} // namespace xel::ui
