# 图形与抗锯齿 TODO

> 本文只记**图形子系统**的后续任务与决策记录；跨领域的欠账见 `docs/todo.md`。
> 已完成项直接删掉，历史看 `git log -- src/graphics src/shaders tools/mtsdf_gen`。

## 现状（一句话）

圆角、胶囊、边框、焦点环、内阴影、图标**全部走距离场**：`kind=1/2` 是解析 SDF（fragment 里算
距离），`kind=3` 是 MTSDF 图集采样。顶点数与半径/DPI 无关，几何细分与顶点 fringe 两条旧路径
已删除（对照见 tag `aa-fringe-imgui`）。

| 图元 | 提交 | 抗锯齿 |
|---|---|---|
| 色块 / 文字 / 位图 | `kind=0` | 无（位图自带） |
| 圆角矩形 / 胶囊 | `kind=1`，1 quad | 解析 SDF + `fwidth` |
| 圆角环（边框 / 焦点环） | `kind=2`，1 quad | 解析 SDF 求交 |
| 图标 | `kind=3`，1 quad | MTSDF `median(RGB)` |

---

## G1 · 编辑器接入图标（最高优先）

`src/cosine/` 目前 **0 处**用 `ui::Icon`，MTSDF 管线只活 `examples/ui_demo` 里。
管线不落地就是玩具。

**做**
- 工具栏 / 资源浏览器 / 播放控制换成 `ui::Icon` + 文字（图标 16~20px）。
- 需要的新图标先进 `tools/mtsdf_gen` 的 `kIcons`，重新烘焙、提交 PNG+JSON。
- 验证：`COSINE_UI_SCALE=2` 下 16px 图标仍清晰；缺图标时的占位方块不破坏布局。

**判据**：编辑器主界面至少 3 处图标走 `kind=3`；截图肉眼对比位图版。

## G2 · 修复 `test_ui_render_snapshot`

- 基线 `kBaselineHash = 0x26d00c69` 早已过期（实际不同）。
- 而且**没有注册进 ctest**（只有 `add_executable`，缺 `add_test`）—— 等于永不执行的测试。

**做**：重录基线 + `add_test(NAME ui_render_snapshot ...)`；确认它和 `test_ui_style_snapshot`
不重复（前者是整页，后者是控件矩阵），必要时二选一或明确分工。

**判据**：`ctest` 数量 +1，且故意改一个像素能让它失败。

## G3 · A 通道：描边 / 发光（MTSDF 的真正卖点）

现在 `kind=3` 只用 `median(RGB)`：

```hlsl
float sd = msdf_median(tex.rgb);          // A 通道（真 SDF）完全没用
```

不用 A 通道的话，选 MTSDF 而不选 MSDF 就没有意义（MSDF 还能省 25% 体积）。

**做**
- 顶点再带一个 `outline_px`（或用 `extra.y` 复用），shader 里：
  `outline_cov = smoothstep(0.5 ± w, alpha_channel + outline_px / (2 * px_range))`。
- 先做一个「同一图标的填充 / 描边 / 发光」三态示例（`ui_demo` 图标页加一行）。
- 注意描边宽度上限受 `px_range` 限制（当前 6px）：要更粗就得重新烘焙更大 range。

**判据**：2px 描边在 16px / 64px 下宽度一致（视觉上是像素级等比，不是按图集分辨率缩放）。

## G4 · 圆角裁剪（需先出方案）

SDF 只解决「画出来的边缘」。所有 `push_clip` 仍是矩形 scissor，所以**圆角面板里滚动的
内容会被切成直角**，与圆角边框不连续。

**候选**
1. stencil：进面板时画一次圆角形状到 stencil，出面板清；会打断批处理。
2. shader 里带圆角 clip：顶点带 `clip_rect + clip_radius`，fragment 里做 `max(orign, -clip_sdf)`；
   与现有 scissor 叠加（scissor 粗裁 + shader 精裁）。保持单 pipeline 与合批。
3. 不做，圆角面板内部避免放滚动内容。

**倾向** 方案 2：改 `PushConstant` 或顶点字段，不需要新 pipeline，不破坏批处理。
但要先量清楚对 vertex 尺寸和指令数的影响。

## G5 · 字体 MSDF（暂缓）

字体目前按整数像素重栅格化（`FontAtlas::quantize_size`），静态清晰度够。
收益只在「大字号连续缩放 / 描边字体」这类场景。

**重启条件**：出现字号动画、或需要描边/发光文字、或 CJK 大字号下看到跳变。

**若做**：`FontAtlas` → 增加 MSDF 模式（RGB 三通道 + A 真距离），
与 G3 共用同一个覆盖率函数；`ui::Paragraph`/`Label` 的绘制入口不变。

## G6 · 图标清单 / SVG 输入

现在只有 7 个内置几何（`rounded_rect` / `circle` / `play` / `chevron` / `arrow` / `plus` / `gear`）。

**做**（按需）
- 先补齐产品真实需要的图标（保存 / 打开 / 撤销 / 重做 / 网格 / 相机 …）。
- 形状复杂到几何难写时接 SVG：`msdfgen-ext` 已有 SVG 解析（需 tinyxml2），
  或引 `nanosvg` 单头在 `tools/mtsdf_gen` 里转成 `msdfgen::Shape`。

**约束**：图集格式 RGBA8 **UNORM**（不能 sRGB）；`margin = padding + px_range` 必须写进 JSON；
新增图标后跑一次方向自检（生成器自带，失败会报错退出）。

## G7 · graphics_demo 的 Canvas 尚未接入 SDF

`examples/graphics_demo/canvas.cpp` 自带一套几何圆角（硬边），与产品路径不同。
它作为「图形实验台」可以独立，但演示的圆角永远不会 AA。

**做**：`Canvas::rounded_rect/outline` 改调 `graphics::append_sdf_rect/ring`；
或反过来用它做「几何 vs SDF」对照页（`sdf` 页已有类似内容）。

**注意**：用户曾明确「graphics_demo 暂时不动」，动之前先确认。

---

## 已决策（避免重复讨论）

| 议题 | 结论 |
|---|---|
| 用 MSAA？ | 不用。要改 swapchain / render target / 离屏全链路，且半透明叠加仍需要覆盖率精度 |
| 用顶点 fringe（ImGui 式）？ | 已实现过又删掉：顶点 1.5~2 倍、分辨率与像素绑定；SDF 是超集 |
| 圆角用 MTSDF 图集？ | 不。解析形状没有分辨率上限，图集只留给图标 |
| 图集走 sRGB？ | 绝对不行，距离会被 gamma 扭曲 |
| 图片槽 vs 新纹理槽 | 图标复用图片槽最后一个（`kMaxImageSlots-1`），不占业务图片号 |
| 旧几何 / fringe 路径保留？ | 已删除，对照看 tag `aa-fringe-imgui` |

## 验证命令

```bash
# 图集烘焙（改了 tools/mtsdf_gen 后必跑，产物要一起提交）
MTSDF_DEBUG=1 ./build/tools/mtsdf_gen --out assets/sdf/ui_icons

# 契约与端到端
./build/test/test_graphics_sdf          # 顶点布局 / 提交 / kind
./build/test/test_graphics_sdf_icon     # 图集加载 + 32/128px 边缘 1px 过渡（需 GPU）

# 视觉
COSINE_EXPORT_PNG=icons.png COSINE_DEMO_SECTION=icons ./build/examples/ui_demo
COSINE_UI_SCALE=2 COSINE_EXPORT_PNG=icons2x.png COSINE_DEMO_SECTION=icons ./build/examples/ui_demo
```
