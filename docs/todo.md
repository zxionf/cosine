# TODO

> 还开放的欠账、风险、暂停池与未决问题，简写汇总。
> 已关闭项与历史见 git、`docs/history.md` 与 `docs/review/`。

## 欠账（R / D 编号）

| 编号 | 内容 | 归属 |
|---|---|---|
| R09 | Windows 实测 | backend |
| R14 | 单窗口 / 单 Renderer 假设 | backend |
| R15 | 文本 shaping：BiDi / 复杂字形 / 字体回退 / IME | ui |
| R17 | 序列化：二进制格式已定，资源引用待 CO-2 | cosine |
| D01 | 圆角抗锯齿：解析 SDF 已覆盖圆角/胶囊/边框/内阴影；图标走 MTSDF 图集（tools/mtsdf_gen 烘焙，kind=3 采样）；字体仍位图 | ui |
| D02 | Table/TreeView 行级背景不参与颜色过渡 | ui |
| D03 | SceneView 椭圆/文字不随 rotation 旋转 | anim/ui |
| D05 | File 菜单 Ctrl+N/O/S 键盘路由 | cosine（CO-2） |
| D06 | 覆盖文件不二次确认、不记忆目录 | cosine |
| D07 | 无自动保存 / 崩溃恢复 | cosine |
| D08 | 属性面板不显示动画求值结果 | cosine |

## 暂停池

| 项 | 挂起原因 | 重启条件 |
|---|---|---|
| 排版（kerning / 避头尾 / justify） | 中文排版够用 | 有排版质量硬需求 |
| 亚像素策略 | 需与 pixel_snap 做矩阵实验 | 有实测数据后 |
| TextSpan 文本片段 | 无富文本需求 | 编辑器行内多色 |
| SDF / MSDF | 逐字号栅格化够用 | 超大字号 / 描边 / 发光 |
| IME preedit | 无中文输入实测 | 真实输入法场景 |
| TreeView 可见行缓存（UI-4） | 节点规模远未到瓶颈 | 节点上千 |
| 弹层出现/消失过渡 | 需 alpha 合成 + 生命周期改造 | 有动画资源时 |
| 缓动曲线可配置 | 四档够用 | 动画需求复杂 |
| SceneView rotation 单测 | 靠截图人工验收 | 旋转逻辑再改时 |

## 风险

| 风险 | 缓解 |
|---|---|
| 平台覆盖：Windows 与 X11 未实测 | 拿到环境先跑冒烟与打包（R09） |
| shader 工具链：slangc 安装不便 | glslc 回退或预编译 .spv |
| 平台差异：Vulkan SDK / GLFW / 字体路径不同 | 发版前两端各跑一次冒烟 |
| 2.5D/3D 返工 | 变换接口不固定 vec2，不写死 2D 假设 |
| 视频导出：编码器/帧同步/色彩空间未定 | CO-3 开工前单独决策 |
| 范围蔓延 | 新控件先问是否服务 cosine；实验不进主树 |
| 示例腐烂 | 示例纳入冒烟门禁（exit=0 / validation=0） |

## 未决问题

- Windows 最低版本与 GPU/驱动范围（决定 CI 与打包）。
- XEL 类型系统边界（动态类型是否够用）。
- 远端 CI 托管平台（发布阶段定）。
- 纹理资源落盘形式与缺失占位策略（CO-2）。

## 有意不做（已决策，避免重评估）

| 项 | 理由 |
|---|---|
| ScrollView 保持二维独立滚动 | ScrollState 是一维，扩成二维会什么都装 |
| Window 标题栏不复用 CollapsibleHeader | 拖动/关闭叉/双击最大化/无标题拖动，套进去更绕 |
| Widget 不拆分 | 显式契约已缓解；拆分回归面覆盖全部控件 |
| menu_bar.hpp 不拆 | 菜单语义的合理复杂度 |
| 性能优化暂缓 | 无实测数据，先 instrumentation（UI-15） |
| TextInput 不接 ScrollState | 只有一条 clamp_scroll，收益为零 |
