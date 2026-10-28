# 20261028-theme-tokens 任务清单

- [x] T01 `srgb_to_linear` / `srgb_hex` 入口，约定「sRGB 书写、线性存储」
- [x] T02 深色令牌重写（shell / surface / control / splitter / text×3 / border）
- [x] T03 浅色令牌同步
- [x] T04 `accent_text` 与 `StyleRole::Accent` 深色文字
- [x] T05 分割条 `splitter` 令牌
- [x] T06 时间轴选中行 0.12 → 0.08
- [x] T07 按钮对比 `#41474F`
- [x] T08 验证：像素抽样一致、ctest 59/59、两处截图目视
- [x] T09 `.design/SPEC.md` 增加「颜色空间与令牌」节并更新 accent 值
- [ ] T10 决策记录、close

## 完成定义

- 写进 theme 的十六进制值等于导出图上的像素值。
- 三栏有可辨边界，按钮与面板分层，强调色当文字时清晰可读。
