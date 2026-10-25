# 发布说明模板

> 复制本文件为 `dist/RELEASE_NOTES-<version>.md` 或粘贴到 GitHub Release。
> 版本号与 `VERSION` / `ui_demo --version` / `project(cosvideo VERSION ...)` 保持一致。

## cosine <version>

发布日期：<YYYY-MM-DD>
构建环境：<Linux / Windows>，<编译器与版本>，Vulkan API <版本>

### 亮点

- <一句话描述本次最重要的变化>

### 新增

- <功能点>

### 变更

- <行为或接口变化>

### 修复

- <缺陷修复>

### 已知问题

- <限制与规避方式>

### 升级说明

- 资源：<assets/ 是否需要更新>
- 字体：<是否需要 COSINE_CJK_FONT>
- 配置：<CMake 选项/环境变量变化>

### 校验

```text
ctest:  <通过数>/<总数>
ASAN:   <通过 / 未执行>
冒烟:   <lavapipe / 手动>
打包:   <产物文件名与大小>
```

### 产物

| 平台 | 文件 | 大小 |
|---|---|---|
| Linux x86_64 | `cosine-<version>-linux-x86_64.tar.gz` | <size> |
| Windows x64 | `cosine-<version>-windows-x64.zip` | <size> |

---

## 发布检查清单

- [ ] 版本号已在 `CMakeLists.txt` 更新
- [ ] `ui_demo --version` 输出正确
- [ ] `ctest` 全绿
- [ ] ASAN 构建与测试通过
- [ ] lavapipe 冒烟通过
- [ ] `docs/project_assessment.md` 状态已更新
- [ ] `docs/dependencies.md` 版本表与实际一致
- [ ] 打包产物可在干净环境解压运行
- [ ] 发布说明已填写产物大小与校验结果
