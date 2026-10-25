# 20261024-m2-gui-mvp 任务清单

## M2-A 事件系统（先做）

- [x] A01 `InputEvent` 补全 MouseDown/Up/Click/Move/Scroll/KeyDown/KeyUp/Char 字段与语义
- [x] A02 `Widget` 的鼠标/键盘处理返回 `bool`，支持消费
- [x] A03 `UISystem` 命中路径 + 从深到浅冒泡
- [x] A04 鼠标捕获：按下目标固定，抬起释放；同目标触发 click
- [x] A05 焦点：`focused_`、`set_focus`、`on_focus_gained/lost`
- [x] A06 Tab / Shift+Tab 在可聚焦控件间循环
- [x] A07 `Application` 注册 key / char 回调并分发
- [x] A08 事件/焦点纯逻辑测试（不依赖 Vulkan）
- [x] A09 HiDPI：窗口光标坐标按 framebuffer/window 比例换算，布局使用 framebuffer 尺寸
- [x] A10 组件 debug 绘制（debug_recursive / append_debug_tree）+ on_hover 钩子 + hover 高亮（默认关闭）

## M2-B 布局

- [x] B01 `Hbox`（与 Vbox 对称）
- [x] B02 `Spacer`
- [x] B03 padding/margin 统一到 Widget/容器（Edges + 容器应用子项 margin）
- [x] B04 尺寸策略：固定、stretch、grow/flex、min/max
- [x] B05 主轴分布 start/center/end/space-between（Vbox/Hbox）
- [x] B06 measure 缓存与 dirty 传播（Vbox/Hbox）
- [x] B07 ScrollView 纵向/横向/双轴（滚动条暂未绘制）
- [x] B08 布局测试覆盖 Vbox/Hbox/Spacer/ScrollView/嵌套、margin、flex、min/max、缓存

## M2-C 控件与主题

- [x] C01 `Theme` 结构与深色主题
- [x] C02 Panel / Frame
- [x] C03 Label / 基础多行文本
- [x] C04 Button（normal/hover/pressed/disabled + 键盘触发）
- [x] C05 Checkbox / Toggle
- [x] C06 Slider（float/int）
- [x] C07 TextInput（单行：光标、选择、删除、输入、Ctrl+A）
- [x] C08 ScrollView / List
- [x] C09 Splitter
- [x] C10 Tooltip（独立浮层，自动 hover 触发待接）

## M2-D 文本与字体

- [x] D01 FontAtlas 改 codepoint 维度，支持 UTF-8 解码
- [x] D02 字形 advance / line height / 文本宽度（基础版）
- [x] D03 自动换行 / 最大宽度 / 裁剪 / 省略号
- [x] D04 TextInput 按 codepoint 维护光标/选择，修复水平滚动后的点击索引
- [x] D05 FontAtlas / TextInput / 换行基础测试
- [x] D06 FreeType 链接与字体发现（Linux/Windows/`COSINE_CJK_FONT`）
- [x] D07 `FontAtlasBuilder`：ASCII + GB2312 一级 + 标点，2048×2048 RGBA8，16px
- [x] D08 字体度量（ascent/descent/line_gap/bearing/advance）与基线排版
- [x] D09 TTF 缺失回退 `default8.png`，sampler 按字体类型选择 linear/nearest
- [x] D10 Renderer 上传运行时生成的图集
- [x] D11 D3 增量字形更新（按需追加 + 纹理更新）
- [x] D12 图集格式、字体来源、许可证、字号策略文档（docs/font_atlas.md）
- [x] D13 CJK/回退/生成性能测试（CJK、动态字形、无字体回退、生成耗时）
- [x] D14 SDF/MSDF 与多字号后续评估（结论：暂不引入，见 docs/font_atlas.md）

## M2-E 渲染与裁剪

- [x] E01 统一 draw list / DrawCmd
- [x] E02 裁剪栈（容器 push/pop scissor）
- [x] E03 ScrollView / TextInput / Panel / Label / Splitter 自动裁剪
- [x] E04 按 texture/scissor 切批
- [x] E05 图层 / z-order：UIRoot overlay 层，Tooltip/弹窗先命中、后绘制
- [x] E06 render target 抽象占位（RenderTarget 描述 + Renderer 按目标 transition/viewport/scissor）
- [x] E07 validation 冒烟（ex4 短时运行）
- [x] E08 裁剪 / draw list 测试

## M2-F 应用层与 demo 收口

- [x] F00 CMake 构建去重：`cosine_text` / `cosine_ui`，CI 默认 `-j2`，测试关闭 module scan
- [x] F01 删除 ex4 手动 scroll->on_scroll/measure/arrange，统一走 UISystem（ex4 已删除）
- [x] F02 Renderer 的 draw_frame / F11 截图 / 空 process_mouse_move 移出应用层
- [x] F03 形成 examples/ui_demo 作为 M2 验收对象
- [x] F04 归档/删除 ex2、ex4、未使用 shader/texture/graphics 头
- [x] F05 清理后的 demo 连续 60 秒 validation 无错误
- [x] F06 demo / UI 集成测试（test_ui_demo）

## M2 验收

- [x] V01 GUI demo：面板、按钮、复选框、滑块、文本输入、滚动列表
- [x] V02 Tab 焦点 + 回车/空格触发按钮
- [ ] V03 1x / 1.5x / 2x HiDPI 表现一致
- [x] V04 60 秒交互 + Vulkan validation 无错误
- [x] V05 布局 / 事件 / 文本测量 / 裁剪测试全绿
- [x] V07 文本支持 UTF-8 + 常用 CJK，换行/省略号正常
- [x] V08 ScrollView / TextInput / Panel 内容不溢出 viewport
- [ ] V06 用户验收后 `sele close 20261024-m2-gui-mvp`

## 当前进度

- M2-A 完成：事件、焦点、HiDPI 光标换算、组件 debug 绘制、on_hover
- M2-B 完成：Hbox、Spacer、padding/margin、flex/grow、min/max、主轴分布、measure 缓存、ScrollView 双轴
- M2-C 完成：Theme、FontAtlas、Label、Panel、Button、Checkbox、Slider、TextInput、List、Splitter、Tooltip；控件已接入 examples/ui_demo
- M2-D 完成：UTF-8、codepoint FontAtlas、换行/省略号、TextInput codepoint、FreeType C2 CJK 图集、D3 动态字形、fallback、字体文档与性能测试
- M2-E 完成：DrawList、scissor 栈、自动裁剪、按 texture/scissor 切批、UIRoot overlay 层（Tooltip/弹窗置顶）、裁剪/图层测试
- 已修复面板不响应：光标坐标按 framebuffer 缩放后与布局坐标一致；debug 树保留但 demo 已关闭，hover 视觉保留
- 评估与问题已归档到 `docs/project_assessment.md`；R01–R18 已纳入阶段计划
- 构建去重：新增 `cosine_text` / `cosine_ui`，共享源只编译一次；CI 默认 `-j2`；测试目标关闭 module scan
- M2-F 完成：examples/ui_demo、demo/UI 集成测试；删除 ex4/ex2、未使用 shader/texture/graphics 头；Renderer 去掉 draw_frame/F11/空 process_mouse_move
- 9/9 ctest 通过（新增 render_target、ui_demo）；ui_demo 运行正常，CJK 与动态字形（囧）均正常显示
- 仅剩 V03（1x/2x HiDPI 实测）与 V06 用户验收
