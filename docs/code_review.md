# cosine / XEL 代码评估与改进清单

> 基线：当前 `main` 分支（`87fc4e1`）的 `src/` 代码。
> 说明：以下结论主要来自静态阅读；`Align` 的恒真问题用最小程序编译验证过。
>
> **环境差异已排除**：服务器上没有 Vulkan SDK、GLM，也没有你本地的字体图集，因此没有在这里做完整构建。
> 下面不会把“服务器缺 Vulkan / 缺 glm / 缺图片”当成项目 bug，只会把它们转成**构建声明和资源管理建议**。
> `first_app.hpp` 和 imgui 是你在本地删除的；imgui 相关的代码/CMake 引用已同步清理（见 4.5），`first_app.hpp` 仍作为待收尾项。

---

## 0. 优先级总览

| 级别 | 含义 | 建议处理顺序 |
|---|---|---|
| **P0** | 会崩溃、渲染错误、交互完全不可用，或构建配置自相矛盾 | 先修 |
| **P1** | 布局/UI 逻辑错误，功能“看起来能跑但行为不对” | 第二批 |
| **P2** | 渲染器健壮性、性能、资源路径、平台可移植性 | 第三批 |
| **P3** | 架构重构、测试、工程化 | 稳定后再做 |

已确认最值得先处理的 6 件事：

1. `Align::stch_x` 拼写错误导致所有子控件恒被横向拉伸；
2. `ScrollView::set_content` 二次析构 / UB；
3. `ScrollView` 不在命中测试链里，真实滚轮事件永远到不了它；
4. SwapChain 失配时 `begin_frame` 提前返回，但 `end()` 仍会 present 旧图像；
5. 所有批次都绑定字体纹理，`WHITE` 描述符从未使用，矩形/线段会采样字体图集；
6. `Rectangle`/`Triangle` 用 `uint16_t` 存索引基址，顶点超过 65535 后会静默画错。

---

## 1. P0：确定会导致崩溃或错误行为的问题

### 1.1 `Align` 的 `stch_x` 拼写错误，导致拉伸判断恒为 true

**位置**

- `src/ui/align.hpp:9`：成员定义成了 `bool stch_x = false;`
- `src/ui/align.hpp:11`：同时存在静态工厂函数 `static constexpr Align stretch_x()`
- `src/ui/vbox.cpp:72`：判断写的是 `if (a.stretch_x)`

**问题**

`a.stretch_x` 匹配到的是**静态成员函数**，不是成员变量。函数指针在布尔上下文中恒为真，因此：

```cpp
if (a.stretch_x) {   // 永远成立
    cs.x = slot.size.x;
    cx = slot.pos.x;
}
```

所有 Vbox 子控件都会被强制拉满宽度，`Align::top_left()/center()/right()` 的横向对齐实际失效。

我用最小程序验证过，编译器给出：

```text
warning: the address of 'static constexpr Align xel::ui::Align::stretch_x()' will never be NULL
```

**建议**

```cpp
// align.hpp
struct Align {
    float x = 0.f;
    float y = 0.f;
    bool stretch_x = false;   // 改回成员
    bool stretch_y = false;

    // 工厂函数换个名字，避免和成员同名
    static constexpr Align h_stretch() { return {0.f, 0.f, true, false}; }
    static constexpr Align v_stretch() { return {0.f, 0.f, false, true}; }
};
```

或者在 `vbox.cpp` 改用 `a.stch_x`，但更推荐把成员和函数彻底改名，并加一条布局单测：

- 子控件 `align = top_left()` 时，`child_slot.size.x == 100`（而不是父宽）；
- 子控件 `align = h_stretch()` 时，`child_slot.size.x == 父宽`。

---

### 1.2 `ScrollView::set_content` 显式析构导致二次析构 / 未定义行为

**位置**：`src/ui/scroll_view.cpp:6-12`

```cpp
void ScrollView::set_content(std::unique_ptr<Widget> c)
{
    if (content_) {
        content_->~Widget();          // 问题在这里
    }
    content_ = std::move(c);
}
```

**问题**

- 显式析构不会释放 `std::make_unique` 分配的内存；
- 随后 `content_ = std::move(c)` 会先 delete 旧指针，对已经析构过的对象再次执行析构并 `free`；
- 如果旧对象是多态类型，显式 `~Widget()` 的行为也与动态类型有关，属于高风险写法。

结果：第二次调用 `set_content` 时就是 UB，通常表现为崩溃或堆损坏。

**建议**

```cpp
void ScrollView::set_content(std::unique_ptr<Widget> c)
{
    content_ = std::move(c);   // unique_ptr 自己会正确释放旧对象
    // 如果之后加 parent 指针，在这里设置
    // if (content_) content_->set_parent(this);
}
```

并补一个测试：连续调用 `set_content` 两次不崩溃。

---

### 1.3 `ScrollView` 在真实事件链中不可达，且没有自动裁剪

**位置**

- `src/ui/scroll_view.hpp`：没有重写 `children()`；
- `src/ui/widget.hpp:33`：基类 `children()` 返回空数组；
- `src/ui/scroll_view.hpp:28`：`hit_testable()` 返回 `false`；
- `src/ui/ui_system.cpp:23`：`deep_hit` 只遍历 `widget->children()`；
- `src/ui/scroll_view.cpp:59-62`：`append_to` 直接解引用 `content_`，没有空指针保护。

**问题**

1. `deep_hit` 看不到 `ScrollView` 的 `content_`，所以鼠标永远命中不到内容；
2. `ScrollView` 自己又不可命中，`hovered_` 会变成 `nullptr`；
3. `UISystem::dispatch_mouse_scroll` 只把事件发给 `hovered_`/`pressed_`，于是 ScrollView 的 `on_scroll` 永远不会被调用；
4. 没有裁剪，内容会画到 viewport 外面，目前只能靠调用方手写 `set_scissor`（`ex4.cpp:81` 还硬编码了 400×600）。

**建议**

```cpp
// scroll_view.hpp
const std::vector<std::unique_ptr<Widget>>& children() const override
{
    static const std::vector<std::unique_ptr<Widget>> empty;
    return content_ ? ... // 这里需要保存“只有一个 child”的容器
}
```

更简单的改法：让 ScrollView 内部持有 `std::vector<std::unique_ptr<Widget>> children_`，`set_content` 把它放进 vector，`children()` 返回它；这样 `deep_hit` 无需特殊处理。或者给 `Widget` 增加 `virtual Widget* child_at(...)`，但短期内前者更省事。

同时：

- `append_to` 先判空：`if (!content_) return;`
- 渲染层增加裁剪栈，`ScrollView::append_to` 自己 push viewport 裁剪，而不是让调用方猜尺寸；
- 让 `on_scroll` 返回 `true` 后停止冒泡（需要先实现事件冒泡，见 1.9）。

`ex4.cpp:78` 目前每帧手动调用 `scroll->on_scroll(...)`，并传固定的 `0.001f`，会绕过事件系统造成“自动滚动”。修好事件链后应删除这段手动调用。

---

### 1.4 SwapChain 失配时仍继续渲染和 present

**位置**：`src/backend/vulkan/renderer.cpp:912-918`、`Renderer::end()`

```cpp
if (result == vk::Result::eErrorOutOfDateKHR) {
    swap_chain_.recreate();
    return;          // 只从 begin_frame 返回
}
```

**问题**

`begin()` 调用了 `begin_frame()`，但 `begin_frame()` 提前返回后，调用方（`ex4`）仍会照常录制绘制，随后 `end()` 仍会：

- 用旧的 `image_index_`；
- 提交到已经不匹配的 swapchain；
- `presentKHR` 使用旧的图像索引。

这会产生验证层错误、画面闪烁，严重时崩溃。

**建议**

把帧状态显式化：

```cpp
bool Renderer::begin();   // false = 本帧无效，调用方应直接 continue

// begin()
if (!begin_frame()) return false;
...
return true;

// end()
if (!frame_valid_) return;
```

`begin_frame()` 返回 `bool`，在 `eErrorOutOfDateKHR` 或重建后返回 `false`；`end()` 开头判断 `frame_valid_`。示例循环也改成：

```cpp
if (app.renderer_.begin()) {
    // draw...
    app.renderer_.end();
}
```

---

### 1.5 SwapChain 重建后，每图信号量数量不匹配

**位置**

- `src/backend/vulkan/renderer.cpp:592-601`：`render_finished_semaphores_` 按创建时的 `swap_chain_.images().size()` 创建；
- `src/backend/vulkan/swap_chain.cpp`：`recreate()` 只重建 SwapChain 和 ImageView，不通知 Renderer。

**问题**

重建后图像数量可能变化（例如从 3 变成 4，或反过来）。之后：

```cpp
render_finished_semaphores_[image_index_]
```

可能越界，或者使用与当前 SwapChain 不匹配的旧信号量。`begin_frame` 里的 `present_complete_semaphores_` 是按 `MAX_FRAMES_IN_FLIGHT` 创建的，不受影响，但 `render_finished` 是每图一个。

**建议**

- 方案 A（改动小）：SwapChain 重建后调用 `Renderer::recreate_sync_objects()`，清空并按新图像数量重建；
- 方案 B（更稳）：改成每帧一个 render-finished 信号量（`MAX_FRAMES_IN_FLIGHT` 个），同时注意“同一图像被连续两帧获取”的已知问题；一般配合 fence 与每帧信号量即可。

无论哪种，都要在重建时把旧信号量清空，不要重复 `emplace_back` 后索引错位。

---

### 1.6 所有批次都绑定字体纹理，`WHITE` 描述符从未使用

**位置**

- `src/backend/vulkan/renderer.hpp:32`：`enum TextureSlot { WHITE = 0, TEXTURE = 1 };`
- `src/backend/vulkan/renderer.cpp:883`：每帧只绑定 `TEXTURE`；
- `src/backend/vulkan/renderer.cpp:create_descriptor_sets`：两组描述符都创建了，但绘制时不用 `WHITE`；
- `graphics::Rectangle`/`Line`/`Triangle` 的 UV 全是 `{0,0}`。

**问题**

片元着色器是：

```slang
return vertIn.fragColor * texture.Sample(vertIn.fragTexCoord);
```

矩形、线段、三角形用 `{0,0}` 采样字体图集左上角。那个像素若是透明，图元就整体不可见；若是黑色/杂色，颜色就会错。`WHITE` 纹理存在的意义就是让非文字图元采样到纯白，但当前没有按批次切换描述符。

**建议**

给 `DrawBatch` 增加纹理槽位：

```cpp
struct DrawBatch {
    ...
    uint32_t texture_slot = 0;   // WHITE / TEXTURE
};
```

`flush_batch` 在纹理槽位或 scissor 变化时切批；`end()` 里每批绑定对应描述符：

```cpp
command_buffer.bindDescriptorSets(..., *descriptor_sets_[frame_index_ * 2 + batch.texture_slot], ...);
```

短期也可以先让矩形使用 `WHITE` 批次，保证图元可见，再逐步扩展多纹理。

---

### 1.7 `uint16_t` 索引基址导致顶点数超限后画错

**位置**

- `src/graphics/rectangle.hpp:20`
- `src/graphics/triangle.hpp:18`

```cpp
uint16_t base = static_cast<uint16_t>(batch.vertices.size());
...
batch.indices.push_back(base);
```

**问题**

渲染器的 `MAX_VERTICES_` 是 `65536 * 4`（262144），索引本身也是 `uint32_t`，但这两个 Drawable 把基址截断成 16 位。UI 有大量矩形时，超过 65535 个顶点后索引会回绕，画出错误的三角形，且没有报错。

**建议**

- 全部改成 `uint32_t base = static_cast<uint32_t>(batch.vertices.size());`
- 在 `append_to` 或 `flush_batch` 里检查 `batch.vertices.size() + 4 <= MAX_VERTICES_`，超限时拆分批次或抛异常；
- 加入一个“大量矩形”的压力测试，确认索引正确。

---

### 1.8 `DrawBatch::scissor` 未初始化就被读取

**位置**

- `src/graphics/drawable.hpp:17`：`glm::vec4 scissor;`
- `src/backend/vulkan/renderer.cpp:664-669`：`set_scissor` 直接比较 `batch_.scissor`

**问题**

GLM 默认构造在未定义 `GLM_FORCE_CTOR_INIT` 时不初始化成员。`begin()` 里 `reset_scissor()` 第一次比较的就是未初始化值，属于 UB，可能错误地跳过/flush 批次。

**建议**

```cpp
glm::vec4 scissor{0.0f, 0.0f, 0.0f, 0.0f};
```

或在 `Renderer::begin()` 里显式 `batch_ = {};` 后再设置完整 scissor，不要依赖默认构造。

---

### 1.9 事件系统没有闭环：按下/抬起不可用，user pointer 类型不一致

**位置**

- `src/xel_application.cpp:9`：`glfwSetWindowUserPointer(..., this)`，存的是 `Application*`；
- `src/backend/window.cpp:framebuffer_resize_callback`：`reinterpret_cast<Window*>`；
- `src/xel_application.cpp:20-24`：`mouse_button_callback` 是空函数；
- `src/ui/ui_system.cpp:37-49`：`dispatch` 只处理 `MouseMove` 和 `MouseScroll`；
- `src/ui/ui_system.cpp:44`：`pressed_` 永远为 `nullptr`。

**问题**

1. user pointer 存 `Application*`，resize 回调却按 `Window*` 解释。因为 `Window` 是 `Application` 的第一个成员，地址碰巧相同，但这依赖对象布局，属于未定义行为；一旦调整成员顺序就会出错。
2. 没有 `MouseButtonDown/Up` 分发，`pressed_` 永远不会赋值，拖动、点击、按住滚动都不可用。
3. `InputEvent::handled/consume()` 定义了但没人使用；事件没有冒泡路径（没有 parent 指针）。
4. `dispatch_mouse_scroll` 同时对 `hovered_` 和 `pressed_` 调用 `on_scroll`，如果两者不同会重复滚动。

**建议**

- 统一 user pointer 的类型：要么存 `Window*`，resize 回调直接使用；要么存 `Application*`，写一个 `Application::framebuffer_resize_callback` 转发到 `app->window_`。不要混用。
- 在 `UISystem::dispatch` 中补：

```cpp
case MouseButtonDown:
    pressed_ = deep_hit(root_.get(), event.mouse_pos);
    if (pressed_) pressed_->on_mouse_down(event.mouse_pos, event.mouse_button);
    break;
case MouseButtonUp:
    if (pressed_) pressed_->on_mouse_up(event.mouse_pos, event.mouse_button);
    pressed_ = nullptr;
    break;
case KeyDown: / KeyUp: / CharInput:
    // 分发给 focused_（需要新增焦点管理）
```

- `Widget` 增加 `on_mouse_down/up/click` 虚函数；
- 增加 `focused_`、`set_focus`、Tab 导航（可以后做）；
- 事件冒泡：给 `Widget` 加 `Widget* parent_`，或让 `UISystem` 在 `deep_hit` 时记录命中路径，从深到浅依次调用，直到返回 `true`。

---

## 2. P1：布局与 UI 逻辑问题

### 2.1 `Vbox::measure` 的间距条件写反，且完全忽略 padding

**位置**：`src/ui/vbox.cpp:49-62`

```cpp
bool first = true;
for (auto& c : children_) {
    if (first) total_h += spacing_;   // 只在第一个之前加了一次
    first = false;
    ...
    total_h += cs.y;
}
```

而 `arrange` 是每个子项之后都加 `spacing_`（`vbox.cpp:82`）。测量和布局结果不一致，滚动内容高度、父容器尺寸都会偏小。

另外 `Vbox` 的 `padding_ = 4.0f`（`vbox.hpp:31`）在 `measure`/`arrange` 中都没有使用。

**建议**

```cpp
float total_h = padding_ * 2;
for (size_t i = 0; i < children_.size(); ++i) {
    if (i > 0) total_h += spacing_;
    total_h += children_[i]->measure(constraints).y;
}
```

`arrange` 也从 `slot.pos + padding_` 开始，宽度约束减去左右 padding。并用一个“3 个 50px 子项 + spacing=4 + padding=4”的单测锁定：期望高度 = 50*3 + 4*2 + 4*2 = 166。

---

### 2.2 `UIRoot` 构造函数写死演示内容，并读取未初始化尺寸

**位置**：`src/ui/root.cpp:9-19`

```cpp
set_size(width, height);
left_column_ = std::make_unique<Vbox>(0, 0, 360, global_.size.y);
...
left_column_->add(std::make_unique<ColorRect>(...));  // 演示内容
```

**问题**

- `global_` 在构造函数中还没被设置，`global_.size.y` 是未初始化值（GLM 默认构造不初始化）；
- 框架的 `UIRoot` 里硬编码三个灰色矩形和 `left_column_`，属于示例代码污染核心类；
- `left_column_` 随后被 `std::move` 进 `children_`，成员变成空指针，容易误用。

**建议**

- 删掉 `left_column_` 和三个 `ColorRect`，`UIRoot` 只负责根节点；
- 演示内容移到 `ex4.cpp` 或 `examples/ui_demo.cpp`；
- 如果确实需要默认背景，用参数传入，不要读 `global_`；
- 构造完成后统一走一次 `arrange`，不要在构造函数里依赖布局结果。

### 2.3 `UIRoot::measure`/`arrange` 与 `layout` 语义不一致

**位置**：`src/ui/root.cpp:35-58`

- `layout()` 遍历子项，调用 `set_pos + layout()`；
- `measure()` 调用子项 `measure`，但返回自己的 `local_bounds().size`，没有汇总子项；
- `arrange(slot)` 把同一个 `slot` 传给**每一个**子项，导致子项重叠。

**建议**

明确 Root 的布局策略（通常是“单子项铺满”或“自由定位”）：

- 若 Root 是自由画布：`arrange` 只设置自己的 `global_`，子项保留自己的坐标；
- 若 Root 是单根容器：只允许一个 child，并让 child 铺满 slot；
- `measure` 汇总子项包围盒（自由画布）或返回约束尺寸（容器）。

同时删除 `UIRoot::layout` 里 `child->set_pos(x()+child->x(), ...)` 这种“读自己再写自己”的写法，改成基于 `global_` 计算，避免多次调用产生累积偏移。

### 2.4 `Widget::parent()` 返回 `unique_ptr`，`add()` 基类空实现

**位置**：`src/ui/widget.hpp:32-34`

```cpp
virtual void add(std::unique_ptr<Widget> child) {}
virtual std::unique_ptr<Widget> parent() const { return nullptr; }
```

`parent()` 返回 `unique_ptr` 无法表达“不拥有父节点”的关系，正确类型应是 `Widget*`。`add()` 在基类静默忽略参数，调用方很难发现子控件没被加入。

**建议**

```cpp
virtual void add(std::unique_ptr<Widget>) = 0;  // 或至少 assert(false)
virtual Widget* parent() const { return parent_; }
```

并在 `add` 时设置 `child->parent_ = this`。如果暂时不想让所有 Widget 都实现 `add`，可以在基类里 `throw std::logic_error("add not supported")`，至少不要静默丢弃。

### 2.5 `rect.hpp` 与 `widget.hpp` 循环 include

**位置**

- `src/ui/widget.hpp:5` include `rect.hpp`
- `src/ui/rect.hpp:3` include `widget.hpp`

当前能编译只是 include 顺序的巧合：先包含 `widget.hpp` 时，`rect.hpp` 被 `pragma once` 挡回，但 `Rect` 已经在返回前定义；反过来先包含 `rect.hpp` 时，`widget.hpp` 使用 `Rect` 时它还没定义。

**建议**

把 `Rect` 拆到独立的 `rect.hpp`，且 `rect.hpp` **不要** include `widget.hpp`；`widget.hpp` 单向 include `rect.hpp`。`rect.hpp` 只需要 `<glm/glm.hpp>` 和 `<cstddef>`。

### 2.6 每帧全量重排，缺少 dirty 标记

**位置**：`src/ui/ui_system.cpp:11-17`

`tick()` 每帧设置 root 尺寸并 `root_->layout()`，没有“布局是否变化”的判断。对当前规模没问题，但一旦 UI 复杂，measure/arrange 会成为热点。

**建议**

- `Widget::set_pos/set_size/add/set_align` 里置 `layout_dirty_`；
- `UISystem::tick` 只在 dirty 或窗口尺寸变化时重排；
- 输入事件导致 hover 颜色变化不应触发重排。

### 2.7 ScrollView 的轴向、方向与约束

**位置**：`src/ui/scroll_view.cpp:44-57`、`scroll_view.hpp:26`

- `axis_` 只保存，没有参与 measure/arrange；
- `offset2_` 从未使用；
- 只处理纵向，横向内容直接溢出；
- 注释说“向下滚 → 内容上移 → offset 增大”，但 GLFW 的 `yoffset` 向上滚为正，通常向上滚应减小 offset；目前 `offset_ += delta * 40` 的方向需要实机确认；
- 没有在窗口 resize / 内容变化后重新 clamp `offset_`。

**建议**

- 明确坐标约定，写进注释并用测试固定；
- 同时支持 `Axis::Vertical/Horizontal/Both`，用 `glm::vec2 offset_` 替换 `offset_ + offset2_`；
- `arrange` 后用 `max_offset = max(0, content_size - viewport)` clamp；
- 给 `on_scroll` 加“是否在 viewport 内”的判定，并返回是否消费。

### 2.8 文本能力有限

`Renderer::draw_char/draw_text` 基于 16×16 的 8px 位图图集：

- 只支持 ASCII，没有 UTF-8；
- 没有换行宽度计算、自动换行、字距/行高配置；
- 图集尺寸要求 `tex_width % 16 == 0`，且固定 `font_cell_px_ = tex_width / 16`；
- 放大到 `24*6` 会明显模糊（`ex4.cpp:74`）。

短期可接受；中期建议把字体度量抽成 `Font` 类，至少支持：

- 字形 advance/line height；
- 按 UTF-8 解码；
- 文本测量 API（给布局用）；
- 可选 SDF 字体。

---

## 3. P2：渲染器健壮性、资源与性能

### 3.1 资源路径依赖当前工作目录

**位置**

- `src/backend/vulkan/renderer.cpp:70`：`read_file("shaders/ui.slang.spv")`
- `src/backend/vulkan/renderer.cpp:269`：`stbi_load("default8.png", ...)`

从仓库根目录启动、从 `build/` 启动、从 IDE 启动，工作目录不同就会失败。CMake 只把 shader 拷到 `build/src/shaders`，字体图集则完全没有拷贝步骤（`src/textures/CMakeLists.txt` 只复制 `texture.jpg`）。

**建议**

- 增加 `assets` 目录，统一放字体图集、纹理、shader；
- CMake 用 `configure_file`/`add_custom_command` 拷到可执行文件旁边；
- 代码里用 `std::filesystem::path(std::filesystem::canonical(argv[0]).parent_path())` 或 CMake 注入 `COSINE_ASSET_DIR`，不要依赖 cwd；
- `read_file` 打开失败时抛出带绝对路径的错误，便于排查。

### 3.2 字体图集没有纳入仓库

`default8.png` 你本地有、仓库里没有。这不是代码 bug，但有两个后续风险：

- 换机器/CI 直接运行时崩溃；
- 图集尺寸、布局（16×16 网格）没有文档，修改字体后 `compute_char_width_ratio` 可能失效。

**建议**

- 把图集作为小资源提交进仓库（如果许可证允许）；或
- 加一个 `tools/gen_font_atlas` 生成脚本，并在 CMake 中构建前生成；
- 在 README 里写清图集格式：尺寸、网格、cell 大小、alpha 通道含义。

### 3.3 批次内存与上传策略

**位置**：`renderer.cpp:652-673`、`813-825`、`upload_vertex_data`

- `batches_.clear()` 每帧销毁内部 vector，容量全部丢失，每帧重新分配；
- `flush_batch` 把 `batch_` 的顶点复制到 `batches_`，再复制到 staging，存在两次全量拷贝；
- 每帧整块 `copyBuffer` 到 device-local，即使只有少量顶点；
- `MAX_VERTICES_`/`MAX_INDICES_` 没有越界检查。

**建议**

- `batches_.clear()` 改为 `batches_.resize(0)` 并保留容量，或使用对象池/arena；
- 直接往 staging 映射内存写，跳过 `batches_` 中间层；批次只记录 `{vertex_offset, index_offset, count, scissor, texture}`；
- 顶点少时使用 host-visible 设备缓冲，避免每帧 transfer；
- 在 `flush_batch` 前检查容量，超限拆批。

### 3.4 截图路径打断帧流水线

`renderer.cpp:944-950` 在 `submit` 之后、`present` 之前调用 `save_swapchain_image_to_png`，内部 `end_single_time_commands` 会 `waitIdle`。功能上可用，但按 F11 当帧会明显卡顿，并且打乱了正常的同步关系。

**建议**

- 把截图放到独立的后处理帧，或记录到一个待处理队列，在下一帧开始前执行；
- 使用独立的 command buffer，避免和帧录制共用 `command_pool_` 的生命周期假设。

### 3.5 验证层回调条件

`src/backend/vulkan/vulkan_context.cpp:debug_callback`：

```cpp
if (severity >= warning || severity >= error)
```

第二个条件冗余，且位掩码用 `>=` 语义不直观。建议：

```cpp
if (severity & (vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eError))
```

另外，Debug 构建强制要求 `VK_LAYER_KHRONOS_validation`，没安装就直接抛异常。可以改成“找不到就打印警告并继续”，避免新机器一上来就跑不起来。

### 3.6 物理设备选择未检查呈现队列

`VulkanContext::is_device_suitable` 只检查 graphics 队列，`create_logical_device` 才找“graphics + surface 支持”的队列族。如果某设备有 graphics 但没有 present，会在创建逻辑设备阶段才失败。

**建议**

在 `is_device_suitable` 里就检查：

```cpp
bool supports_graphics_and_present = std::ranges::any_of(queue_families, [&](auto const& qfp) {
    // 需要能拿到 queue family index，遍历索引
});
```

或者在 `pick_physical_device` 后再做一次显式校验，并给出清晰错误。

### 3.7 `Renderer::begin()/end()` 缺少状态校验

如果调用方忘记 `begin()` 或连续 `end()`，当前会操作未初始化的 command buffer / `frame_index_`。建议加 `bool in_frame_` 状态：

```cpp
assert(!in_frame_); in_frame_ = true;   // begin
assert(in_frame_);  in_frame_ = false;  // end
```

并在 `~Renderer` 里 `device().waitIdle()`（虽然 `vk::raii` 会清理，但显式等待更安全，尤其涉及持久映射内存）。

### 3.8 `read_file` / 图像加载缺少校验

- `read_file` 没有检查 `file.read` 是否成功；
- `create_texture_image` 在 `stbi_load` 失败时先计算了 `image_size`（使用了未定义的宽高），虽然随后抛异常，但顺序不清晰；
- 没有检查 `tex_channels` / 图集尺寸是否符合 16×16 网格。

**建议**：先判空，再计算尺寸；对 `tex_width % 16 != 0` 显式报错；`read_file` 检查 `gcount`。

### 3.9 帧缓冲尺寸与窗口尺寸混用（HiDPI）

`Window::get_extent()` 返回的是 `width_/height_`（窗口尺寸），而 `framebuffer_resize_callback` 写入的是 framebuffer 尺寸。在高 DPI 屏幕上两者可能不同，UI 布局使用的是 `get_extent()`，光标坐标是窗口坐标，渲染映射用的是 SwapChain extent，三者可能不一致。

**建议**：UI 布局统一使用 framebuffer 尺寸（与 SwapChain extent 一致），光标坐标按 `framebuffer/window` 比例缩放；或明确只支持 1:1 缩放并记录限制。

---

## 4. 构建与工程化

### 4.1 GLM 依赖没有在 CMake 中声明

你在本地有 GLM，所以能编译；但项目本身没有：

```cmake
find_package(glm CONFIG REQUIRED)
target_link_libraries(ex4 PRIVATE glm::glm)
```

也没有 FetchContent 拉取。换一台机器（或 CI）就会缺头文件。

**建议**（任选其一）：

- 系统安装 + `find_package(glm CONFIG REQUIRED)`；
- FetchContent 固定版本；
- 作为 `third_party/glm` submodule。

并在 README 写明最低版本（建议 0.9.9.8 以上）。

### 4.2 `main.cpp` 仍引用已删除的 `first_app.hpp`

`src/main.cpp:1`：

```cpp
#include "first_app.hpp"
```

你已经删除了 `first_app.hpp`，但 `src/CMakeLists.txt:4-7` 仍然构建 `ex0`。结果默认构建会失败。

**建议**

- 若 `ex0` 不再需要：从 CMake 删除 `ex0`，或把 `main.cpp` 改成使用现有 `xel::Application`；
- 若 `first_app` 只是暂时未提交：提交它，或明确 `ex0` 为可选目标（`option(BUILD_EX0 OFF)`）。

### 4.3 `find_package(Vulkan)` 在 `add_subdirectory(shaders)` 之后

`src/CMakeLists.txt:1-3`：

```cmake
add_subdirectory(shaders)
add_subdirectory(textures)
find_package(Vulkan REQUIRED)
```

`src/shaders/CMakeLists.txt:51` 使用 `Vulkan_GLSLC_EXECUTABLE`，但此时变量还没定义。`.slang` 走 slangc，不受影响；`.vert/.frag` 会生成错误命令。

**建议**

```cmake
find_package(Vulkan REQUIRED)
add_subdirectory(shaders)
add_subdirectory(textures)
```

并在 shaders 子目录里对 `Vulkan_GLSLC_EXECUTABLE` 做 `if(NOT ...) message(FATAL_ERROR ...)`，不要静默生成空命令。

### 4.4 硬编码 slangc 路径

`src/shaders/CMakeLists.txt:10-15`：

```cmake
set(SLANGC_EXECUTABLE "/home/zxionf/tools/slang/bin/slangc")
if(WIN32)
    set(SLANGC_EXECUTABLE "D:/Program Files/slang-.../slangc.exe")
endif()
```

这种回退在别人机器上必然失败，而且 configure 阶段不报错。

**建议**

- 优先 `find_program(SLANGC_EXECUTABLE slangc)`；
- 支持 `-DSLANGC_EXECUTABLE=...` 覆盖；
- 找不到时 `message(FATAL_ERROR "slangc not found; install Slang or pass -DSLANGC_EXECUTABLE=...")`；
- 若某些示例不再需要 slang，可把 shader 编译设为可选。

### 4.5 `test` / `movy` 对已删除 imgui 的引用（已处理）

imgui、`ImGuiColorTextEdit`、`movy/` 以及 `test/testimgui.cpp`、`test/testdocking.cpp` 已删除，相关构建引用也已同步清理：

- 根 `CMakeLists.txt` 移除了 `add_subdirectory(movy)`；
- `test/CMakeLists.txt` 移除了 `imgui/...` 源文件以及 `testimgui`/`testdll`/`testdocking` 目标；
- `.gitignore` 移除了失效的 `/imgui/examples`；
- 保留了 `testglfwinit` 和解释器测试（`test_interpreter_*`），它们不依赖 imgui。

### 4.6 `ex1.cpp` 已失效，`ex3.cpp` 未纳入构建

- `ex1.cpp` include 的 `backend/vulkan_context.hpp`、`backend/swap_chain.hpp`、`xel_pipeline.hpp`、`xel_model.hpp` 都不存在；
- `ex3.cpp` 是独立 OpenGL 测试，CMake 中被注释掉；
- `ex0`、`ex2`、`ex4` 各自复制 backend 源文件列表，新增文件容易漏。

**建议**：

- 删除 `ex1.cpp`，或迁移到 `examples/legacy/` 并修好 include；
- `ex3.cpp` 移到 `tools/`，不要放在 `src/`；
- 把 backend 编译成静态库 `cosine_backend`，各示例只链接它；
- 示例统一放 `examples/`，每个示例一个 CMake 目标。

### 4.7 两套构建脚本已过期

`src/compile.sh` / `src/compile.bat` 只编译 `simple_shader`，且硬编码 Vulkan SDK 路径，和现在的 CMake + slang 流程不一致。建议删除，或在 README 中明确“仅历史遗留，不再维护”。

### 4.8 FetchContent 使用 `stb` 的 `master`

根 `CMakeLists.txt:43-49`：

```cmake
FetchContent_Declare(stb GIT_REPOSITORY ... GIT_TAG master)
```

不可复现，某天上游改动可能直接破坏构建。建议固定 commit/tag，并加 `GIT_SHALLOW`。

### 4.9 GLFW 强制 Wayland、关闭 X11

根 `CMakeLists.txt:11-13`：

```cmake
set(GLFW_BUILD_WAYLAND ON ...)
set(GLFW_BUILD_X11 OFF ...)
```

在 X11 环境、部分 CI、WSL 上无法构建或运行。建议做成 option：

```cmake
option(COSINE_USE_WAYLAND "Build GLFW with Wayland" ON)
```

或同时开启 X11 + Wayland，让 GLFW 自己选择。

### 4.10 缺少 README 构建步骤和 src 测试

README 只有两行，没有：

- 依赖（Vulkan SDK、glm、freetype、slangc/glslc、GLFW 后端）；
- 构建命令和运行目录；
- 资源放置说明；
- 已知限制。

`test/` 目前只覆盖解释器，UI 的 measure/arrange/hit-test、批处理、裁剪都没有测试。

**建议**：

- README 增加“Quick start”；
- 给 `src/ui` 加轻量单测（不需要 Vulkan，只测纯布局逻辑）；
- CI 至少跑 `cmake -B build && cmake --build build`（可用软件 Vulkan/lavapipe）。

---

## 5. 架构与设计建议

### 5.1 合并/删除重复渲染器

`src/backend/vulkan/font_renderer.cpp`（748 行）与 `renderer.cpp`（1274 行）大量重复，且 `font_renderer` 已经**没有任何引用**（`ex2` 用的是 `Renderer`）。建议：

- 删除 `font_renderer.*`；
- 如果其中还有独有能力（例如旧的字体布局），先把需要的部分移植到 `Renderer`，再删除。

### 5.2 把示例绘制从 Renderer 移出

`Renderer::draw_frame()`（`renderer.cpp:827`）里是三角形、文字、纹理的演示内容；`save_swapchain_image_to_png`、F11 处理也属于应用逻辑。建议：

- `Renderer` 只负责：帧生命周期、批次、管线、资源绑定；
- 演示内容放到 `examples/` 的 `main`；
- 截图放到应用层工具函数。

### 5.3 统一绘制模型

目前有两条路径：

- `Renderer::draw_rect/draw_line/draw_text` 直接写 `batch_`；
- `graphics::Drawable::append_to` 写 `DrawBatch`，再由 `Renderer::draw` 转发。

建议统一为“绘制列表（draw list）”：

```cpp
struct DrawCmd {
    uint32_t vertex_offset;
    uint32_t index_offset;
    uint32_t index_count;
    Rect scissor;
    TextureId texture;
};
```

UI 只负责生成 draw list，Renderer 负责排序/合批/上传。这样纹理切换、裁剪、批次拆分都有明确位置，也方便以后做 GPU 渲染或离屏渲染。

### 5.4 UI 事件模型

当前只有“全局 hover”，缺少：

- parent/冒泡；
- 鼠标捕获（拖动时目标不随光标改变）；
- 焦点与键盘；
- 裁剪区域参与命中测试（内容超出 viewport 不应可点）；
- 事件消费返回值。

建议最小实现：

```cpp
Widget* hit_path[16];  // 或 std::vector<Widget*>
// deep_hit 时记录路径，事件从最深开始向上传递，直到 consume
```

并让 `hit_testable` 与“是否参与事件”分离：容器可以不接收鼠标，但它的子项应该可以被命中。

### 5.5 布局系统

建议逐步引入：

- `measure(constraints) -> size`：只测量，不改状态；
- `arrange(slot)`：只定位，不再调用子项 `measure`（或缓存测量结果）；
- `layout_dirty_`：输入变化时标脏；
- padding/margin 统一由布局容器处理，不在 `arrange` 里临时补；
- 加一个 `LayoutTest`，覆盖 Vbox、Root、ScrollView 的边界情况。

### 5.6 “动画引擎”核心仍缺失

目前 `src` 是“2D 图元渲染器 + UI 雏形”，还没有：

- 时间轴/关键帧/缓动；
- 场景图/图层/合成；
- 资源管线（纹理、字体、音频、视频）；
- 与 `interpreter` 的绑定；
- 撤销/重做、序列化。

`interpreter` 本身结构不错（lexer/parser/evaluator 分层、有测试），但和渲染引擎完全独立。建议先把引擎侧稳定到“能加载脚本 → 创建图元 → 播放时间轴”，再扩展语言功能；不要在渲染/UI 还有 UB 的时候继续堆功能。

### 5.7 建议的目录结构

```text
cosine/
  CMakeLists.txt
  cmake/
  assets/                 # 字体图集、纹理、shader 源
  src/
    core/                 # 时间轴、场景、动画
    renderer/             # Vulkan 后端、批次、资源
    ui/                   # Widget、布局、事件
    app/                  # Application、窗口、输入
  examples/
  interpreter/
  tests/
```

---

## 6. 建议的修复路线

### 第一阶段：先让它稳定（建议 1–2 天）

- [ ] 修 `Align::stch_x` / `stretch_x` 命名冲突，补布局测试
- [ ] 修 `ScrollView::set_content` 二次析构
- [ ] `ScrollView` 重写 `children()`，补空指针保护和裁剪
- [ ] `begin_frame` 返回 `bool`，`end()` 跳过无效帧
- [ ] SwapChain 重建时重建每图信号量
- [ ] 批次按 `WHITE/TEXTURE` 绑定描述符
- [ ] `uint16_t` 索引基址改 `uint32_t`
- [ ] 初始化 `DrawBatch::scissor`
- [ ] 统一 user pointer 类型，补鼠标按下/抬起分发

### 第二阶段：把 UI 交互做对（建议 2–4 天）

- [ ] `Widget` 增加 parent、`on_mouse_down/up`、焦点
- [ ] 事件从最深命中向上冒泡，返回是否消费
- [ ] `Vbox` 修正 spacing/padding，`UIRoot` 移除演示内容
- [ ] `ScrollView` 支持轴向、方向、resize 后 clamp
- [ ] `Rect`/`Widget` 拆分循环 include
- [ ] HiDPI 坐标统一

### 第三阶段：工程化与可移植（建议 1 周）

- [ ] CMake 声明 glm，固定 stb 版本
- [ ] `find_package(Vulkan)` 提前，slangc/glslc 缺失时 fail fast
- [ ] 删除/归档 `ex1`、`compile.sh/.bat`、`font_renderer`
- [x] 清理 `test`、`movy` 中的 imgui 残留
- [ ] 字体图集纳入仓库或构建生成
- [ ] backend 抽成库，示例统一到 `examples/`
- [ ] README 写清依赖、构建、运行、资源路径
- [ ] 加 UI 布局单测和最小 CI

### 第四阶段：引擎能力（稳定后再做）

- [ ] 统一 draw list 与批处理
- [ ] 文本测量/换行/UTF-8
- [ ] 时间轴、关键帧、缓动
- [ ] 场景/图层/合成
- [ ] xel → 引擎的绑定 API
- [ ] 序列化与资源管线

---

## 7. 附：几条可以顺手加的检查

1. 编译时打开警告，先清掉明显的隐式截断和未使用变量：

   ```cmake
   target_compile_options(ex4 PRIVATE -Wall -Wextra -Wconversion -Wshadow)
   ```

   仅 `Align` 这一处就会立刻暴露。

2. Debug 构建配合 Vulkan validation layer 跑一遍 `ex4`，重点看：

   - `vkCmdDrawIndexed` 的索引越界；
   - `presentKHR` 的 `VUID-vkPresentInfoKHR-pImageIndices`；
   - 信号量/围栏的使用；
   - `vkCmdBindDescriptorSets` 与布局匹配。

3. 给 `ScrollView` 和 `Vbox` 写纯逻辑测试，不需要 Vulkan/窗口：

   ```cpp
   Vbox box{0, 0, 400, 600};
   box.add(std::make_unique<ColorRect>(...));
   auto size = box.measure({400, 600});
   assert(size.y == 期望值);
   ```

4. 每次修一个 P0，就在 `docs/code_review.md` 里把对应条目标记完成，避免文档和代码脱节。

---

**最后一句**：这个项目当前最有价值的是 Vulkan/UI 的骨架和解释器分层，不建议现在继续扩展“动画引擎”功能面。先把上面 P0/P1 修完、让构建和交互稳定，再决定是继续做 UI 框架，还是把精力转到时间轴/场景/脚本绑定。
