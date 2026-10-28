# 20261028-preview-ruler 任务清单

- [x] T01 `ui::PreviewRuler`：比例映射、刻度自适应、播放头、右侧时间标签
- [x] T02 点击 / 拖动 seek，越界夹取；未按下时移动不触发
- [x] T03 `editor::PreviewPane`：主体弹性 + 标尺固定 28px
- [x] T04 工具栏 `◀ Frame` / `Frame ▶`（1/30s，先暂停再 seek）
- [x] T05 `tick` 同步标尺时间与 duration（仅在变化时重设）
- [x] T06 `test_ui_preview_ruler`（5 组）+ 注册，ctest 58/58
- [x] T07 冒烟 exit=0 / validation=0；导出确认标尺与标签
- [ ] T08 决策记录、SPEC 折叠、close

## 完成定义

- 预览底部可见时间标尺，点击或拖动即 seek，播放头随播放移动。
- 工具栏可单帧前进 / 后退，且逐帧时自动暂停。
