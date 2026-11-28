// 分区注册表 + 各分区构建函数的声明。
//
// 新增一个分区需要三件事：
//   1. 在 sections/ 下实现 build_xxx(Demo&, Vbox&)；
//   2. 在本文件声明它；
//   3. 在 demo.cpp 的 registry() 里加一条 {id, title, summary, &build_xxx}。
// 外壳代码（布局/主题/状态栏）不需要改动。

#pragma once

#include "demo.hpp"

namespace demo::sections
{
void build_overview(Demo& demo, ui::Vbox& column);
void build_basics(Demo& demo, ui::Vbox& column);
void build_inputs(Demo& demo, ui::Vbox& column);
void build_text(Demo& demo, ui::Vbox& column);
void build_layout(Demo& demo, ui::Vbox& column);
void build_containers(Demo& demo, ui::Vbox& column);
void build_data(Demo& demo, ui::Vbox& column);
void build_overlays(Demo& demo, ui::Vbox& column);
void build_media(Demo& demo, ui::Vbox& column);
void build_icons(Demo& demo, ui::Vbox& column);
} // namespace demo::sections
