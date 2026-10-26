# 20261026-image-widget

<!-- 为什么改、边界与不属于本次的范围 -->

## Image 控件（占位实现）

### 为什么

M3.5 组件扩充只剩 `Image`。真实纹理采样依赖 M6 的资源管线（图片解码、上传、描述符、采样器），
但布局层现在就需要一个「图片位」：能声明尺寸/比例/来源，参与容器排布，在 UI 里占位与验收。
先给出稳定的公开接口，等 M6 把 `texture_id` 接到真实纹理时，控件本身不用改。

### API（`src/ui/image.hpp`，header-only）

- `Image(float width, float height)` / `Image(glm::vec2 size)`。
- `set_source(std::string)` / `source()`：图片路径，仅供 M6 加载使用，当前不读文件。
- `set_texture_id(std::uint32_t)` / `texture_id()`：预留纹理句柄，`0` 表示无纹理。
- `set_fit(Fit)` / `fit()`：`Fill` / `Contain` / `Cover`，当前只存储，M6 采样时生效。
- `set_tint(glm::vec4)` / `tint()`：叠加色，占位绘制用它着色。
- `set_placeholder_text(std::string)` / `placeholder_text()`：占位文案。
  未显式设置时取 `source()` 的文件名，仍为空则回退 `"image"`。
- 尺寸规格复用 `Widget`：`set_size_percent` / `set_aspect_ratio` / `min_size` / `max_size`。

### 绘制（占位）

- 圆角底色（`accent` 降低不透明度）+ 1px 边框（`panel_border`）。
- 对角交叉线，明确表示「尚未绑定纹理」。
- 居中占位文案（`small_text_size` + `text_muted`），裁剪在控件内。
- 即使 `texture_id != 0` 也保持同样的占位绘制——真实采样在 M6 接入，
  避免出现「看起来支持纹理、实际是空图」的误判。

### 验证

新建 `test/test_ui_image.cpp` 并注册：

- 构造尺寸、`measure`、`arrange` 后 bounds 正确。
- `source` / `texture_id` / `fit` / `tint` 存取。
- 未设置占位文案时：有来源取文件名、无来源回退 `"image"`。
- 在 `Vbox` 中声明 `size_percent` 与 `aspect_ratio` 时按父内容区解析。
- `append_to` 不崩（无字体上下文时静默跳过文字）。

gallery `Layout` 分区：两张占位图（一张有来源、一张空），验证百分比宽与 16:9。

### 边界（本次不做）

- 不做图片解码、纹理上传、采样（M6 资源管线）。
- 不做九宫格拉伸、平铺（tile）、mipmap、色彩空间处理。
- 不做图片缓存与异步加载。
