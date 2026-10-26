# 20261026-image-widget 交接

## 结果

- 新增 `Image` 占位控件：`src/ui/image.hpp`（header-only，无需改 CMake 源列表）。
  - `source` / `texture_id` / `fit(Fill|Contain|Cover)` / `tint` / `placeholder_text`。
  - 占位文案回退链：显式文案 → `source` 文件名（兼容 `/` 与 `\`）→ `"image"`。
  - 绘制：圆角底色（accent 10%）+ 1px 边框 + 对角交叉线 + 居中文件名（裁剪在控件内）。
  - 即使 `texture_id != 0` 也保持占位绘制，真实采样留给 M6。
- gallery `Layout` 新增：两张 16:9 占位图（宽 50%，一张有来源、一张空来源）。
- 新增 `test/test_ui_image.cpp`（4 个用例），注册为 `ui_image`。

## 验证

- `ctest --test-dir build`：40/40；`ctest --test-dir build-asan`：40/40。
- gallery 冒烟 Layout 分区（默认 / classic / 滚动 1400）：`validation=0`。
- `sele lint` 四项 ok。

## M3.5 归档

- 阶段计划中 `Image` 已勾选，并在 M3.5 段落加入归档说明（2026-10-26）。
- 组件扩充、样式系统、交互打磨、文本显示、布局补充（Grid / Wrap / 百分比与宽高比 /
  绝对定位与 z-index）全部完成。
- `STATE.md`：Phase = M3.5 完成并归档；Next = M4（启动前确认范围与属性系统边界）。

## 后续

- M6 资源管线就绪后，把 `Image::append_to` 换成真实纹理采样（`texture_id` → 描述符绑定），
  接口无需变动。
- backlog（图集治理、纹理上传、排版细节等）转入 M4 之后按需处理。
