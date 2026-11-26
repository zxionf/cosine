# sele 0.21.4（Windows 构建）：`sele git` 子命令全部失败

> 报告对象：sele 上游
> 复现环境：Windows 11 (10.0.26300)，Git for Windows 2.47.0，sele 0.21.4
> 影响面：`sele git status/diff/log/commit/stash/branch/health/reconcile` 全部不可用，
> 且 `sele check` / `sele status` / `sele cache rebuild` 这类内部要读 HEAD 的命令也会多打印一行错误。

## 1. 现象

```console
$ sele git status --short
The system cannot find the path specified.
The system cannot find the path specified.
$ echo $?
4

$ sele git commit --task T41 --step S1 --message "..."
The system cannot find the path specified.
The system cannot find the path specified.
Error: git add failed:

$ sele git reconcile --sync-to 67218170c09e1722c5c85cd7aa427f1ce913dfda
The system cannot find the path specified.
The system cannot find the path specified.
Error: Unknown commit: 67218170c09e1722c5c85cd7aa427f1ce913dfda
```

特征：

- 每条命令打印**恰好一条** `The system cannot find the path specified.`（`sele health` 4 条、`commit` 2 条）——
  条数等于该命令内部调用 `git` 的次数。
- stdout 永远为空，退出码 4。
- `sele git commit` 的错误信息是空的（`git add failed:` 后面没有内容）。
- `.sele/config.json` 的 `git_mode` 已是 `write`，不是只读拦截。
- `sele check`、`sele status`、`sele plan/task/step` 等不依赖 git 的命令正常。

## 2. 根因

`sele` 用 `_popen()` 调 git，但命令是按 **POSIX shell 语法**拼的：

```
cmd /c git -C '.' rev-parse HEAD 2>/dev/null
cmd /c git -C '.' rev-parse --abbrev-ref HEAD 2>/dev/null
cmd /c git -C '.' merge-base --is-ancestor <sha> <sha> 2>/dev/null
cmd /c git -C '.' status --short 2>/dev/null
```

`_popen` 在 Windows 上经 `cmd.exe /c` 执行，而 cmd.exe 不支持这两种写法：

| sele 生成 | cmd.exe 的解析结果 |
|---|---|
| `-C '.'`（单引号） | cmd 不处理单引号，git 收到字面量 `'.'`（含引号） |
| `2>/dev/null` | cmd 把 `/dev/null` 当成路径 `\dev\null`；该路径不存在 → **命令直接失败，git 根本不会启动** |

失败发生在 cmd 的**重定向解析**阶段，所以：

- 看不到 git 自己的报错（git 没跑）；
- 报错信息是 cmd 的 `ERROR_PATH_NOT_FOUND` 文案；
- 每条命令只报一次（每条都带一个 `2>/dev/null`）。

## 3. 证据

### 3.1 直接对照（同一台机器、同一 PATH）

```console
$ cmd /c git -C '.' status --short 2>/dev/null
The system cannot find the path specified.
The system cannot find the path specified.
# → 无 git 输出

$ cmd /c git -C "." status --short 2>NUL
M  docs/ci.md
M  src/ui/style.hpp
...
# → 正常
```

### 3.2 截获 sele 的真实命令行

把 `COMSPEC` 指向一个会记录 `argv` 的垫片，再运行 `sele git status`：

```
[SHIM] [/c] [git] [-C] ['.'] ['rev-parse'] ['HEAD'] [2>/dev/null]
[SHIM] [/c] [git] [-C] ['.'] ['rev-parse'] ['--abbrev-ref'] ['HEAD'] [2>/dev/null]
[SHIM] [/c] [git] [-C] ['.'] ['merge-base'] ['--is-ancestor'] [<sha>] [<head>] [2>/dev/null]
[SHIM] [/c] [git] [-C] ['.'] ['status'] ['--short'] [2>/dev/null]
```

注意：参数里**单引号是内容的一部分**，且 stderr 重定向是 POSIX 形式。

### 3.3 排除项（以下都不影响，均实测）

| 假设 | 验证 | 结果 |
|---|---|---|
| `git_mode: read` 拦截 | 改成 `write` | 仍失败 |
| PATH 里找不到 git | `git` 在 `D:\Program Files\Git\cmd\git.exe`；分别试 `Git\cmd`、`Git\mingw64\bin`、Windows 原生 PATH | 仍失败 |
| 仓库/`.sele` 结构问题 | 在干净的全新 git 仓库（无 `.sele`）跑 `sele git status` | 仍失败 |
| sele 缺 DLL | PATH 只放 Windows 路径 → `0xC0000139`（缺 `libstdc++`/`libgcc_s`）；加 MinGW 工具链目录后可启动 | 与 git 失败无关 |
| `_popen` 在本机不可用 | 自写 MinGW 程序 `_popen("git status --short")` | **正常返回**并触发 PATH 上的 git 垫片 |
| sele 是否真的启动了子进程 | 把记录型 `git.exe` 放进 PATH 首位，运行 `sele git status` | **零记录**（子进程未创建）→ 与 3.2 的结论一致 |
| Shell 环境（`SHELL`/`COMSPEC`/`TEMP`） | 逐个覆盖/清除 | 仍失败 |
| 在 WSL 里跑 | WSL bash + interop 调用同一个 `sele.exe` | 仍失败（因为内部仍是 Windows `_popen`→`cmd.exe`） |
| `--root` 形式 | `D:\...`、`D:/...`、带尾斜杠、默认 `.` | 均无效 |

## 4. 影响

- `sele git *` 全部不可用；
- 因为内部读 HEAD 的命令也会失败，**`git.commit` / `git.drift` 事件无法写入 `log.jsonl`**，
  `git-fingerprint.json` 长期停留在旧 commit（本仓库曾停在 `1feca6c`，而实际 HEAD 已前进两次）；
- 进而出现"漂移被检测到但无法 `reconcile`"的死锁（本次实际遇到 D3 drift）。

## 5. 本地绕过（临时，不建议长期保留）

把 `COMSPEC` 指向一个垫片，把 sele 生成的命令翻译成 cmd 能懂的等价形式后再交给真正的 cmd：

```
'…'           → "…"
2>/dev/null   → 2>NUL
```

垫片核心（MinGW/C，约 30 行）：

```c
for (int i = 2; i < argc; ++i) { join(cmd, argv[i], " "); }   /* 重组命令行 */
for (const char* p = cmd; *p; ++p) {
    if (*p == '\'')                   { append(fixed, "\""); continue; }
    if (startsWith(p, "2>/dev/null")) { append(fixed, "2>NUL"); p += 10; continue; }
    append_char(fixed, *p);
}
_spawnl(_P_WAIT, "C:\\WINDOWS\\system32\\cmd.exe", "cmd.exe", "/c", fixed, NULL);
```

用 `set COMSPEC=<shim>` 运行 sele 后，`sele git status/commit/reconcile` 全部恢复正常（已验证提交成功）。

注意副作用：垫片必须放在**不在 PATH、也不是当前目录**的位置，否则 `git` 会解析到垫片自己；
本次调试中曾因在垫片目录内运行 sele 导致 `git` 被解析成记录型垫片，输出了假 HEAD，
把 `deadbeef` 写进了 `git-fingerprint.json`，需要用 `sele git reconcile --sync-to <真实 HEAD>` 修正。

## 6. 建议修复

Windows 构建下应区分平台拼装命令，任选其一：

1. **参数数组直连**（最稳）：用 `CreateProcess` 传 `lpApplicationName = git.exe` + 参数数组，
   完全绕开 shell；顺带消除引号与注入面。
2. **按平台选 shell 语法**：Windows 用 `"%s"` 双引号 + `2>NUL`；POSIX 保持现状。
3. 若坚持经 shell，至少在 Windows 上不做 stderr 重定向（或改 `2>NUL`），并把 `-C <path>` 的引号换成双引号。

另外两点小建议：

- `sele git commit` 失败时把底层 stderr 一起带出来（现在 `git add failed:` 后面是空的，很难定位）；
- `reconcile` 在拿不到 HEAD 时，错误信息区分"git 不可用"与"commit 不存在"——本次两者都报 `Unknown commit`，误导排查方向。

## 7. 环境信息

```
OS            Windows 11 10.0.26300
Shell         Git for Windows 2.47.0 (MINGW64_NT-10.0-26300)
sele          0.21.4  (D:\Program Files\sele\bin\sele.exe, MinGW/UCRT 动态链接)
git           2.47.0.windows.2  (D:\Program Files\Git)
Compiler      g++ 15.2.0 (x86_64-posix-seh-ucrt) —— 用于编译诊断垫片
```
