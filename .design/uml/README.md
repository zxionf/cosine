# UI 层架构图（PlantUML）

`src/ui/**` 的结构图，一图一文件（每个渲染器都能单独打开）。文本版说明见
`docs/ui_architecture.md`。

| 文件 | 图 | 内容 |
|---|---|---|
| `01-packages.puml` | 图1 | 包结构与依赖方向、架构断言 D1–D5 / K1–K8 |
| `02-node.puml` | 图2 | `Node`（= `Widget`）：12 组能力 + 4 个内嵌部件 + `MeasureCache` |
| `03-controls.puml` | 图3 | 控件分类与继承；`ListView` 等行为基元 |
| `04-layering.puml` | 图4 | `Layer{Content,Floating,Modal}` 与 `z_index` 的排序契约 |
| `05-layout-pipeline.puml` | 图5 | 失效传播 → measure → arrange → layout → 绘制 |
| `06-event-dispatch.puml` | 图6 | 命中链 → 冒泡 → 焦点；模态作用域；安全移除 |
| `07-modal.puml` | 图7 | 模态栈 `modal_frames_` 与 Scrim/z 归一化 |
| `08-theme.puml` | 图8 | 主题 / 样式过渡 / 语义投影 |

## 渲染

```bash
# 全部（每个文件产出一张同名 png）
plantuml -tpng .design/uml/*.puml

# 单张
plantuml -tpng .design/uml/04-layering.puml
```

- 需要 PlantUML ≥ 1.2020；中文用 `Microsoft YaHei`，非 Windows 环境可自行改
  `skinparam defaultFontName`。
- 每个文件只有一个 `@startuml … @enduml`，`@startuml` 前与 `@enduml` 后没有任何内容
  （部分 IDE 插件对此敏感）；修改时请保持这个结构。
