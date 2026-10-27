# 字体图集与文本渲染

> 基线：M2-D（2026-10-24）；M4.6 T1–T4、T8 更新（2026-10-27）
> 实现：`src/graphics/font_atlas.*`、`font_atlas_builder.*`、`font_face.*`
> 资源：`assets/fonts/default8.png`、`assets/fonts/gb2312_level1.txt`

## 1. 目标与选择

- 字体风格：统一 TTF（A2）；`default8.png` 仅作 TTF 缺失 fallback。
- 来源：系统字体优先，`COSINE_CJK_FONT` 覆盖；打包阶段可选内置 OFL 字体。
- 覆盖：ASCII + GB2312 一级汉字 3,755 + 常用标点（C2）。
- 图集：固定 2048×2048 RGBA8，按绘制字号逐档栅格化（字形 key = 字号 + codepoint）；
  空间不足时该字形可重试（不永久记入 missing），仍不足则回退基准字号缩放绘制。
- 生成：FreeType 运行时生成并上传。
- 动态：按需追加字形，只上传图集的脏矩形（M4.6 T2）。
- 度量：ascent / descent / line height / bearing / advance，按基线排版；
  行高保证不小于 `ascent + |descent|`（M4.6 T4）。
- 多字号：已实现逐档栅格化（`4..256`）；SDF/MSDF 暂不引入（M4.6 T12）。

## 2. 字体来源

查找顺序：

1. `COSINE_CJK_FONT` 环境变量
2. `COSINE_CJK_FONT_FACE` 环境变量（TTC face index，默认 0）
3. Linux 候选：
   - `/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc`（SC face index 2）
   - `/usr/share/fonts/noto-cjk/NotoSansCJK-Medium.ttc`
   - `/usr/share/fonts/adobe-source-han-sans/SourceHanSansCN-Regular.otf`
   - `/usr/share/fonts/wenquanyi/wqy-zenhei/wqy-zenhei.ttc`
4. Windows 候选：
   - `C:/Windows/Fonts/msyh.ttc`
   - `C:/Windows/Fonts/msyhbd.ttc`
   - `C:/Windows/Fonts/simhei.ttf`
   - `C:/Windows/Fonts/simsun.ttc`

全部失败时加载 `default8.png`。

许可证提醒：

- Noto Sans CJK / Source Han Sans：OFL，适合随包分发。
- WenQuanYi Zen Hei：使用前确认其许可证与分发要求。
- 系统字体默认不随项目分发；打包阶段需单独决策。

## 3. 覆盖范围

`gb2312_level1.txt` 包含 GB2312 一级汉字 3,755 个。运行时还会加入：

- ASCII `0x20–0x7E`
- 常用标点：`，。！？：；“”‘’（）【】《》、…—·￥`

缺失字形按需加载；字体确实不包含的字符回退到 `?`。

## 4. 图集格式

- 尺寸：2048×2048
- 格式：RGBA8，纹理格式 `eR8G8B8A8Srgb`
- 原点：左上角，`v=0` 在顶部
- 像素：**预乘 alpha**，`RGB = A`
  - 原因：当前混合状态为 `One / OneMinusSrcAlpha`（预乘 alpha）
  - 透明背景 `RGB=0`，避免字形四边形变成白色方块
- 白色像素：`(0,0)` 保持 `255,255,255,255`，供矩形/线段采样
- padding：每个字形周围 1px，避免线性采样渗色
- packing：简单 shelf packing，按行填充
- 采样：TTF 图集 linear；像素 fallback nearest
- 地址模式：`ClampToEdge`

## 5. 字形度量

每个 `Glyph` 记录：

- `u0/v0/u1/v1`：图集 UV
- `advance_ratio`：advance / pixel_size
- `width_ratio` / `height_ratio`：bitmap 尺寸 / pixel_size
- `bearing_x_ratio`：bitmap_left / pixel_size
- `bearing_y_ratio`：bitmap_top / pixel_size

排版：

- 行顶 `y` → 基线 `y + ascent_ratio * size`
- 字形左上角 = `pen_x + bearing_x * size`，`baseline - bearing_y * size`
- 行高 = `line_height_ratio * size`，其中 `line_height_ratio = max(字体 height, ascent + |descent|, 1.0)`
  （`FontAtlas::normalize_metrics`，M4.6 T4），保证行框容得下降部
- 换行按行高递增，不再假设行高等于字号

`default8.png` fallback 使用：

- `ascent_ratio = 1`
- `bearing_y_ratio = 1`
- `width/height` 取字格度量

## 6. 动态字形与纹理上传

- `FontAtlas::ensure_glyph(codepoint, pixel_size)`：
  - 已存在 → 直接返回
  - 缺失且存在 loader → 调用 `FontFace::add_glyph`
  - loader 失败且**不是**空间不足 → 记入 `missing_`（永久缺字）
  - loader 失败但**是**空间不足（`last_add_out_of_space()`）→ 不记入 `missing_`，后续可重试
  - 最终仍缺字时回退 `?`
- `FontAtlas::revision()` 在字形表变化后递增（供渲染层判断是否需要同步）
- `FontAtlas::take_dirty_rect(x, y, w, h)`：取出「自上次调用以来新增/改写」的包围盒并清零；
  空白字形（无位图）不产生脏区
- 渲染层每帧比较 revision，有变化且存在脏区时：
  - 只按脏矩形分配 staging（逐行 `memcpy`）
  - `VkBufferImageCopy` 用 `imageOffset = {x, y}`、`imageExtent = {w, h, 1}`、`bufferRowLength = 0`
  - 单字形上传量从 16MB 降到几十 KB（M4.6 T2）
- 脏区是**单个包围盒**：一帧内多处新增会合并成一个大矩形，尚未做多矩形拆分

## 7. 字号策略

- 图集按**绘制尺寸**栅格化：`glyphs_` 的 key 是 `(量化后的整数像素字号, codepoint)`。
- `FontAtlas::quantize_size()` 把字号四舍五入到整数像素并限制在 `4..256`，
  避免相同的视觉字号因浮点差异反复生成。
- `FontFace::add_glyph(atlas, cp, pixel_size)` 会先 `FT_Set_Pixel_Sizes` 到目标尺寸再渲染，
  因此每个字号都经过 FreeType 的像素网格拟合，不依赖放大基准字形。
- 图集空间不足时 `add_glyph` 返回 false，但**不**记入 `missing_`（可重试）；
  查找会自动回退到基准字号（`base_pixel_size`，默认 16）缩放绘制，保证文字仍可见。
- `used_ratio()` 返回 shelf packing 的已用高度占比，可用于监控水位与判断是否回退缩放。
- 首次出现某个字号时会追加字形，并只上传该字形的脏矩形（与动态 CJK 追加同一路径）。
- 已知取舍：多字号共存会占用图集，超大字号（≥96px）大量 CJK 仍可能撑满；
  字形淘汰 / 图集重建 / 分页图集尚未实现（需要字符集清单与多纹理槽）。
- SDF/MSDF 评估结论：暂不引入
  - 逐字号栅格化已能保证清晰度
  - SDF 需要额外生成管线、边缘/字重调优
  - 在需要任意缩放动画或高 DPI 混合时再评估

## 8. DPI 与像素比例（M4.6 T8）

- `FontAtlas::set_pixel_ratio(ratio)`：字形按 `字号 × ratio` 的**物理像素**栅格化，
  绘制尺寸仍是逻辑字号；投影按同一比例放大，因此高 DPI 下不糊。
- `glyph_pixel_size(size)` 是唯一入口；`quantize_size` 上限 `4..256`，避免 2 倍缩放下大字被截断。
- 由 `COSINE_UI_SCALE=auto|<倍数>` 驱动（0.25–4，默认 1.0）：
  - `root` 尺寸 = `framebuffer / ratio`
  - 输入坐标 = `framebuffer / ratio`
  - push constant 用逻辑尺寸；scissor 从逻辑乘回物理
- 字形 key 含物理字号，因此同一逻辑字号在 1x / 2x 下是两组独立字形，可共存。

## 9. 已知限制

- 没有 HarfBuzz shaping：Latin kerning/ligature、BiDi、Indic 不处理
- 没有 emoji/彩色字体支持
- 没有 IME preedit / 候选框：只处理提交后的 UTF-8 codepoint
- 没有 SDF/MSDF
- 脏矩形是单一包围盒；一帧内多处新增会合并，未做多矩形拆分
- 多字号共存会占用图集空间，暂无字形淘汰；不足时回退基准字号缩放
- 运行中切换 DPI 不自动重算，需重启
- Windows 字体查找与运行尚未实测

后续优化清单（kerning、避头尾、TextSpan、字体回退、IME、SDF、图集淘汰等）见
`.design/阶段计划.md` 的「M4.6 文本 / 图集 / 输入 backlog」T5–T13。
