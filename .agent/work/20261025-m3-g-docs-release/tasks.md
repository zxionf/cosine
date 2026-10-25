# 20261025-m3-g-docs-release 任务清单

## M3-G 文档与发布说明

- [x] G01 `docs/index.md`（阅读顺序 + 文档地图）
- [x] G02 `docs/dependencies.md`（依赖版本、用途、升级方式）
- [x] G03 `docs/readme_draft.md`（README 建议稿，不改根 README）
- [x] G04 `docs/release_notes_template.md`
- [x] G05 更新 `docs/project_assessment.md` 基线到 0.3.0
- [x] G06 同步阶段计划与任务状态；全量回归

## 插入：M3.5 UI 打磨（新增计划）

- [x] P01 `.design/阶段计划.md` 插入 M3.5（组件扩充、样式、交互、布局、UI 动画、demo）
- [x] P02 `examples/ui_gallery`：分区展示全部现有控件与交互
- [x] P03 `test_ui_gallery`：分区切换、tooltip、滚动、调试面板状态
- [x] P04 打包/冒烟脚本覆盖 `ui_gallery`；`docs/packaging.md`、`docs/ci.md` 同步
- [x] P05 `docs/index.md`、`docs/project_assessment.md` 更新 M3.5 与 gallery 入口
- [x] P06 主题切换：`Theme::light()`、`UIRoot::set_theme`、`UISystem::set_theme`、Renderer 清屏色、gallery 按钮与 `COSINE_GALLERY_THEME`、`test_ui_theme`
- [x] P07 新控件：`TabBar`、`Radio`/`RadioGroup`、`Progress` + `test_ui_widgets` + gallery Inputs 分区
- [x] P08 ScrollView 可见滚动条（拖拽/翻页/预留空间）+ `test_ui_scrollbar`
- [x] P09 Dropdown（overlay 遮罩 + 列表、键盘导航、Esc 关闭）+ `UIRoot::remove_overlay` + `test_ui_dropdown`

## 当前进度

- docs：index / dependencies / readme_draft / release_notes_template 已落地
- M3.5 计划已插入；ui_gallery、主题切换、TabBar/Radio/Progress、ScrollView 滚动条、Dropdown 完成
- 19/19 ctest 通过（新增 test_ui_scrollbar、test_ui_dropdown）
