#pragma once

#include <cstdint>
#include <string_view>

namespace xel::ui
{
// 控件「是什么」的语义标签（UI-52）。
//
// 用途：让通用代码（调试面板、Tab 顺序、测试断言、未来的无障碍导出）不必
// dynamic_cast 就能知道一个控件是什么、能不能点、叫什么名字。
//
// 注意：这是**给外部看的语义**，与内部实现分类（LeafNode / ContainerNode / ListView）
// 是两件事——Panel 既可能是一个 Vbox，也可能是一个自绘容器。
enum class WidgetRole : std::uint8_t
{
    Custom,
    Panel,
    Button,
    CheckBox,
    Radio,
    Slider,
    TextField,
    Label,
    Image,
    List,
    Table,
    Tree,
    Menu,
    MenuItem,
    Dialog,
    Tooltip,
    Splitter,
    Progress,
    ScrollView,
    TabView,
};

// 语义投影（UI-52）：**派生**出来的只读视图，不存第二份真值。
//
// 规则：除 role/label/checkable（控件常量）外，所有状态都从 Interaction 现读。
// 禁止给 Semantics 加 setter，否则它会变成第二个状态所有者。
struct Semantics
{
    WidgetRole role = WidgetRole::Custom;
    std::string_view label;   // 控件名/无障碍名（指向控件自己的字符串，调用方不持有）
    bool enabled = true;
    bool focused = false;
    bool selected = false;
    bool checkable = false;   // 可勾选/可选中（CheckBox / Radio / 列表行）
    bool expanded = false;    // 展开态（树节点、折叠面板、下拉菜单）
    bool editable = false;    // 可编辑（TextField）
};

inline const char* to_string(WidgetRole role)
{
    switch (role) {
        case WidgetRole::Custom:     return "custom";
        case WidgetRole::Panel:      return "panel";
        case WidgetRole::Button:     return "button";
        case WidgetRole::CheckBox:   return "checkbox";
        case WidgetRole::Radio:      return "radio";
        case WidgetRole::Slider:     return "slider";
        case WidgetRole::TextField:  return "textfield";
        case WidgetRole::Label:      return "label";
        case WidgetRole::Image:      return "image";
        case WidgetRole::List:       return "list";
        case WidgetRole::Table:      return "table";
        case WidgetRole::Tree:       return "tree";
        case WidgetRole::Menu:       return "menu";
        case WidgetRole::MenuItem:   return "menuitem";
        case WidgetRole::Dialog:     return "dialog";
        case WidgetRole::Tooltip:    return "tooltip";
        case WidgetRole::Splitter:   return "splitter";
        case WidgetRole::Progress:   return "progress";
        case WidgetRole::ScrollView: return "scrollview";
        case WidgetRole::TabView:    return "tabview";
    }
    return "custom";
}

// 角色属性（UI-55）：由 role 纯派生，不需要每个控件各写一个虚函数。
inline bool role_is_checkable(WidgetRole role)
{
    return role == WidgetRole::CheckBox || role == WidgetRole::Radio;
}
inline bool role_is_editable(WidgetRole role) { return role == WidgetRole::TextField; }
} // namespace xel::ui
