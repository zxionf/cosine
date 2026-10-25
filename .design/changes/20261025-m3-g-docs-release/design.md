# 20261025-m3-g-docs-release

<!-- 为什么改、边界与不属于本次的范围 -->

## 文档结构

- 新增 `docs/index.md` 作为文档入口：阅读顺序、按主题的文档地图、外部依赖版本表。
- 新增 `docs/dependencies.md`：第三方依赖、版本/提交、用途、升级方式与平台前提。
- 新增 `docs/readme_draft.md`：README 建议稿。**不直接修改根 README.md**（由用户定稿后自行替换）。
- 新增 `docs/release_notes_template.md`：发布说明模板，配合 `VERSION` / `--version` / 打包脚本使用。

## 状态收口

- `docs/project_assessment.md` 更新基线到 0.3.0，并标注 M3 已完成项与剩余项。
- `.design/阶段计划.md` 与对应 tasks 同步 M3-G 完成状态。
- SPEC 由 `sele close` 折叠，不手工编辑。

## 边界

- 不改 `README.md`、`AGENTS.md`、`.agent/CONTRACT.md`。
- 不写营销文案；README 草稿只描述事实、构建、运行与限制。
- 不归档 `docs/code_review.md`（历史评审记录保留）。
