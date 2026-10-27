#pragma once

#include "scene.hpp"
#include "timeline.hpp"

namespace xel::anim
{
// 项目 = 场景 + 时间轴。是整个编辑器的数据根对象。
// 序列化见 serialize.hpp，可撤销编辑见 command.hpp。
struct Project
{
    scene::Scene scene;
    Timeline timeline;
};
} // namespace xel::anim
