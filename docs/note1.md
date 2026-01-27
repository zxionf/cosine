#### imgui窗口
```bash
// 禁止用户操作
ImGuiWindowFlags_NoTitleBar      // 无标题栏
ImGuiWindowFlags_NoResize        // 禁止调整大小
ImGuiWindowFlags_NoMove          // 禁止移动
ImGuiWindowFlags_NoScrollbar     // 无滚动条
ImGuiWindowFlags_NoScrollWithMouse // 禁止鼠标滚轮滚动
ImGuiWindowFlags_NoCollapse      // 禁止折叠/最小化
ImGuiWindowFlags_NoBackground    // 无背景
ImGuiWindowFlags_NoSavedSettings // 不保存到 ini 文件
ImGuiWindowFlags_NoFocusOnAppearing // 出现时不获取焦点
ImGuiWindowFlags_NoBringToFrontOnFocus // 聚焦时不置顶

// 用户交互
ImGuiWindowFlags_NoMouseInputs   // 禁止所有鼠标交互
ImGuiWindowFlags_NoNav           // 禁止键盘/游戏手柄导航
ImGuiWindowFlags_NoNavInputs     // 禁止导航输入
ImGuiWindowFlags_NoNavFocus      // 禁止导航聚焦
ImGuiWindowFlags_NoInputs        // 禁止所有输入

// 窗口布局
ImGuiWindowFlags_AlwaysAutoResize // 自动调整大小以适应内容
ImGuiWindowFlags_NoSavedSettings  // 不保存窗口位置/大小
ImGuiWindowFlags_MenuBar          // 显示菜单栏
ImGuiWindowFlags_HorizontalScrollbar // 显示水平滚动条
ImGuiWindowFlags_NoDecoration     // = NoTitleBar|NoResize|NoScrollbar|NoCollapse
ImGuiWindowFlags_AlwaysUseWindowPadding // 总是使用窗口内边距

// 窗口状态
ImGuiWindowFlags_NoNavFocus       // 禁止自动获取导航焦点
ImGuiWindowFlags_UnsavedDocument  // 显示未保存标记（*）
ImGuiWindowFlags_NoDocking        // 禁止停靠

// 子窗口特定
// 子窗口专用标志
ImGuiWindowFlags_ChildWindow      // 创建为子窗口
ImGuiWindowFlags_Tooltip          // 工具提示窗口
ImGuiWindowFlags_Popup            // 弹出窗口
ImGuiWindowFlags_Modal            // 模态对话框
ImGuiWindowFlags_ChildMenu        // 子菜单
```