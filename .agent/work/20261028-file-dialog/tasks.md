# 20261028-file-dialog 任务清单

- [x] T01 `anim::save_project_file` / `load_project_file`（落盘字节 = `serialize()`）
- [x] T02 `ui::FileBrowser`：目录优先 + 名字升序 + 扩展名过滤 + `..` + 滚动
- [x] T03 单击 `on_select` / 双击 `on_activate` / 目录变更回调
- [x] T04 编辑器 Open / Save As 对话框（`Dialog` + `show_dialog` overlay）
- [x] T05 File 菜单：New / Open / Save / Save As 接真逻辑
- [x] T06 `COSINE_EDITOR_OPEN=1` 便于离屏截图验证布局
- [x] T07 测试：serialize 文件 I/O 2 组 + FileBrowser 4 组，ctest 59/59
- [x] T08 冒烟 exit=0 / validation=0；导出确认对话框
- [ ] T09 决策记录、SPEC 折叠、close

## 完成定义

- 能从菜单保存项目到 `.cos` 文件，再打开回来，节点与关键帧一致。
