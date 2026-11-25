# Spec

> 当前设计事实：只写现状。历史见 git、`docs/history.md` 与 `docs/review/`。

## 项目定位

- 近期：通用 2D GUI 框架（自研 retained-mode UI，Vulkan，Linux Wayland + Windows）。
- 中期：脚本驱动动画（XEL）、时间轴关键帧编辑器、2D 动画导出（离屏 + 视频编码）。
- 长期：2.5D / 3D。
- 非目标：3D 网格/光照/物理、音频/视频解码、插件系统、多人协作、完整 IDE、即时模式 GUI。

## 技术栈

- C++20；CMake（>= 4.0）；backend 收敛为静态库。
- Vulkan（vulkan-hpp RAII），仅 Vulkan 后端，当前渲染 2D。
- GLFW：Linux Wayland + Windows，X11 为可选项。
- GLM、STB（固定 commit）、FreeType。
- Shader：Slang 为主，glslc 回退。
- UI：自研 Widget 树，不引入第三方 UI 库。
- 脚本：XEL 解释器（token/lexer/parser/AST/evaluator/environment）。

## 约束

- Vulkan-only；当前 2D，接口不写死 2D 假设。
- Linux Wayland 与 Windows 一级目标；资源统一 `assets/`，按 `COSINE_ASSET_DIR` → 可执行文件旁 → 构建期注入定位。
- 依赖保持最小集；新增依赖先记决策。
- 视频导出走外部编码器（ffmpeg）管道。
- Debug 构建必须过 Vulkan validation；资源/初始化失败给出明确错误。

## 分层与目录

```
src/backend/    window（GLFW+DPR）、vulkan_context、swap_chain、renderer、asset_path
src/graphics/   DrawList/DrawBatch/Vertex、图元、FontAtlas、FrameStats
src/ui/         Widget 树、布局、事件、焦点、控件、主题、UISystem
src/anim/       scene/property/timeline/easing/player、Command/CommandStack
src/cosine/     编辑器（cosine_editor + cosine 可执行）
src/events/     InputEvent、KeyCode/Modifier
interpreter/    XEL 解释器（与 UI 解耦，可嵌入）
examples/       ui_demo / ui_gallery / ui_anim_demo / ui_anim_showcase / ui_lab / moss / font_lab
test/           CTest
assets/         shader、字体
```

依赖单向：`graphics` 不依赖 backend；`ui` 依赖 `graphics` + `events`；`backend` 消费 `graphics` 的 DrawList。

## UI 架构

### Widget 树与布局

- `children_` + `adopt()` 是唯一挂载入口；`parent` 非拥有指针。
- 布局契约：`measure(constraints)` / `arrange(slot)`；`fixed_size()` 忽略约束；`fills_parent()` 铺满父容器。
- `measure()` 返回**控件自身**尺寸，不得返回内部派生量（如 `ScrollView` 的内容区）；否则父级按小尺寸摆放，`arrange()` 里会再扣一次。
- 主轴（`Vbox`/`Hbox`）尺寸由 `ui/layout_budget.hpp` 的 `resolve_main_axis` 统一解算：
  `pct`/`fill` 先认领预算（总和按认领额压缩到 ≤ 可用），再按 `flex` 分配剩余
  （正增长 / 负收缩，夹在 `min/max`），`margin` 全轴参加计价。
- `stretch` 与 `size_percent`/`fill`/`fixed_size` 互斥；拉伸结果重新 `clamp`，
  `aspect_ratio` 与 `clamp` 一起求解（`apply_aspect_clamp`）。
- 叶子控件用构造期 `preferred_size` 作自然尺寸，`measure()` 不读 `local_.size`（避免拉伸后只涨不缩）。
- 可见性三态 `Display{Flow, Positioned, Hidden}`：`set_visible(false)` 等价 Hidden（不占位），
  `set_absolute(true)` 等价 Positioned（不占流式空间但仍 arrange）。
- 溢出策略 `Overflow{Visible, Clip, Scroll}`；`clips_children()` 由它派生，`content_overflows()` 报告溢出。
- `TabView::measure` 只算当前页；`Splitter::measure` 由 pane 自然尺寸推导，分配尊重子项 `min_size`。
- measure 缓存键是（constraints, 布局轮次）：同轮内 `measure`/`arrange` 共用一次测量，跨轮必然重算；
  `set_theme` 递归失效；`arrange` 只用 `set_geometry` 写几何（不标脏），脏标记传播带早退。
- `UIRoot` 是自由画布：子控件保留局部坐标；顶层同时声明 `fill_x+fill_y` 的子项（应用外壳 Vbox）会直接铺满 root。
- 裸容器（Vbox/Hbox/Grid/Wrap）也跟随 `theme.padding/spacing`，显式 `set_padding/set_spacing` 仍优先。
- 布局字面量收进 `Theme`（dialog/window/splitter/section 一组度量），随 density 缩放。
- 布局内核：`LinearLayout(axis)` 被 `Vbox`/`Hbox` 复用；`ui/layout_utils.hpp` 收敛内容区与 Positioned 子项摆放。
- `Widget::LayoutSpec` 是 align/flex/pct/aspect/fill 的只读快照 + 回写转发；`Align::baseline_align()` 支持文字基线对齐。

### 层级与排序

- `Layer{Content, Floating, Modal}`；`Widget::layer_paint_order()` 是跨 `children_`/`overlays_` 的唯一排序入口。
- 排序键：layer 升序 → z 升序（稳定）；绘制正序、命中反序、Tab 正序；`z_index` 仅层内生效。
- 归属：普通控件 `Content`；`Tooltip`/`MenuPopup`/`MenuScrim`/`DropdownList` `Floating`；`Scrim`/模态 `Window`/`DialogHost` `Modal`。

### 模态

- `UIRoot` 维护 `ModalFrame{window, scrim, prev_focus}` 栈；`layout()` 归一化 z（frame i → scrim 2i+1 / window 2i+2）并剔除悬空帧。
- `push_modal(window, with_scrim)`：快照焦点 + 聚焦 `preferred_focus()` → 首个可聚焦 → 窗口自身。
- `pop_modal`：恢复快照（仍在树中且不在被弹出子树内才恢复，否则清空）。
- `dispatch_key`/`dispatch_char` 限定最上层模态子树；`DialogHost` 经 `push_modal(host, with_scrim=false)` 纳入作用域。
- `Scrim`：全屏遮罩（`alpha`/`layer` 可配，`alpha=0` 作透明遮罩），吞掉指针/滚轮/点击/右键/双击/长按，不参与 Tab；`set_on_dismiss` 在点击/滚轮时触发。
- `Window::set_modal` 的置顶与 Esc 是**临时**的：打开前快照，`release_modal` 时还原（不粘住）。

### 窗口

- `Window` 是固定尺寸的浮层：`fixed_size()=true`（`arrange` 只用 slot 定位，不拿 slot.size 覆盖自身）；`clips_children()=true`（绘制与命中裁剪一致）；支持拖动 / 8 向 resize / 折叠 / 最大化。
- 几何统一由 `Window::chrome()` 派生：**标题高 = 字体 line box 高度**（不加 padding），`glyph`/`icon`/`inset`/`resize_margin`/`keep_visible` 同源；**绘制与命中共用这份几何**；折叠/展开图标两臂均为 45°。
- `set_content_inset(px)`（默认 **0**）：内容相对窗口边框的内缩，纯视觉布局。
- resize 命中带由 `theme.resize_margin`（默认 **4px**）决定：不占用内容布局，靠 `Widget::resize_edges_at(pos)` + `UISystem` 在派发 down 前扫描，**优先于内容子控件命中**；hover 到边缘时切换 GLFW resize 光标。
- 置顶是插入序语义：`raise()` 把自己移到父容器子项末尾（`Widget::raise_child`），不改写兄弟的 `z_index`。
- 关闭：`close()` 只隐藏；`destroy()` 交给 `on_destroy_`，未注册时从 root 延迟移除；`request_close()` 先问 `on_close_`，通过后**延迟到下一次 `layout()`** 才真正 `destroy()`（避免事件派发期间销毁自身）。
- Esc 只在 `esc_closable()` 为真时关闭（模态窗口自动开启，普通窗口默认关闭）。
- 模态期间的置顶与 Esc 是临时的：打开前快照，`release_modal` 时还原。

### 滚动与列表

- `ScrollState`：一维滚动（值/目标/平滑逼近，Smooth/Fast/Line 三档）。
- `ListView`：行列表行为基元——可见行窗口、像素对齐（`snapped_offset`=round）、`append_rows`、`row_at`、`draw_row` 契约。
- `Table` / `TreeView` 继承 `ListView`（滚动、行循环、取整口径收敛到基类）。
- `ScrollView`：独立的二维滚动（不接 `ScrollState`）。

### 缩放与坐标

- 坐标三跳：GLFW 窗口坐标 → `Window::cursor_to_framebuffer`（×DPR）→ `UISystem::framebuffer_to_logical`（÷ui_scale）→ UI 逻辑坐标。
- `Application::set_ui_scale` 是单一入口，同步 `ui_scale_`/renderer/uisys/字体栅格化比例；运行时变化触发重布局。
- `UISystem::pointer_pos()` 提供已缓存的逻辑坐标（tooltip 等每帧跟随用）。

## 渲染

- `DrawList`/`DrawBatch`：顶点、scissor、纹理槽、透明度栈。
- 纹理槽：0=White、1=字体图集、2–15=图片（`texture_id` = 槽索引 + 1）。
- 离屏：`Renderer::render_offscreen` + `write_png`。

## 事件模型

- `InputEvent`（鼠标移动/按下/抬起/滚轮/键盘/字符）。
- 命中返回根到目标的路径；事件从目标向上冒泡，`bool` 返回值表示是否消费。
- 鼠标按下捕获 `pressed_`；抬起仍在同一目标触发 `click`；右键/双击/长按为独立手势。
- 焦点由 `focused_` 管理；Tab/Shift+Tab 循环；焦点环显示时机由 `FocusPolicy`（Keyboard/Always/Never）决定。

## 文本

- `FontAtlas`：UTF-8、CJK、动态字形、逐字号栅格化、DPI 缩放、脏矩形上传。
- 字体来源：`COSINE_CJK_FONT` → 系统字体（Noto CJK / Source Han Sans / 微软雅黑…）→ `default8.png` 回退。

## 动画

- `scene`（Scene/Node/NodeId）、`property`（PropertyId/PropertyValue）、`timeline`（Timeline/Track/Keyframe）、`easing`、`player`。
- `Command`/`CommandStack` 撤销栈。
- 序列化：二进制 `.cosine`（确定性、自校验、可演进），见 `docs/project_format.md`。

## 工程

- 版本号唯一来源：根 `CMakeLists.txt` 的 `project(cosine VERSION ...)`。
- 测试：`ctest` + ASAN + lavapipe 冒烟。
- 打包：Linux `tar.gz` + Windows `zip`（内置 `ui_demo` 与 `ui_gallery`），见 `docs/packaging.md`。
- 任务管理：`sele`（`sele status` / `sele tree`）；`.sele/` 只通过 sele 命令读写。
