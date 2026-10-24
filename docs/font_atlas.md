# 字体图集与文本渲染

> 基线：M2-D（2026-10-24）
> 实现：`src/graphics/font_atlas.*`、`font_atlas_builder.*`、`font_face.*`
> 资源：`assets/fonts/default8.png`、`assets/fonts/gb2312_level1.txt`

## 1. 目标与选择

- 字体风格：统一 TTF（A2）；`default8.png` 仅作 TTF 缺失 fallback。
- 来源：系统字体优先，`COSINE_CJK_FONT` 覆盖；打包阶段可选内置 OFL 字体。
- 覆盖：ASCII + GB2312 一级汉字 3,755 + 常用标点（C2）。
- 图集：固定 2048×2048 RGBA8，16px 基准；其他字号先按比例缩放。
- 生成：FreeType 运行时生成并上传。
- 动态：D3 按需追加字形 + 更新纹理。
- 度量：ascent / descent / line height / bearing / advance，按基线排版。
- SDF/MSDF、多字号：后续阶段评估，不在 M2 实现。

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
- 行高 = `line_height_ratio * size`
- 换行按行高递增，不再假设行高等于字号

`default8.png` fallback 使用：

- `ascent_ratio = 1`
- `bearing_y_ratio = 1`
- `width/height` 取字格度量

## 6. 动态字形（D3）

- `FontAtlas::ensure_glyph(codepoint)`：
  - 已存在 → 直接返回
  - 缺失且存在 loader → 调用 `FontFace::add_glyph`
  - 失败 → 记录 missing，回退 `?`
- `FontAtlas::revision()` 在字形追加后递增
- Renderer 在每帧 `end()` 前比较 revision
- 有变化时重传整张 2048×2048 图集纹理
- 当前为整图重传；后续优化为 dirty-rect 局部更新

## 7. 字号策略

- M2 使用 16px 单图集
- 其他字号按比例缩放
- 24/32px 如果视觉效果不足，再增加 24px 第二图集
- SDF/MSDF 评估结论：暂不引入
  - 当前 UI 以 16px 为主，缩放可接受
  - SDF 需要额外生成管线、边缘/字重调优
  - 在需要大字号/缩放或高 DPI 时再评估

## 8. 已知限制

- 没有 HarfBuzz shaping：Latin kerning/ligature、BiDi、Indic 不处理
- 没有 emoji/彩色字体支持
- 没有 IME preedit / 候选框：只处理提交后的 UTF-8 codepoint
- 没有 SDF/MSDF
- 动态字形更新当前整图重传
- Windows 字体查找与运行尚未实测
