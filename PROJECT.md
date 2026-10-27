# PROJECT.md

Project-specific context that supplements `AGENTS.md`.

`AGENTS.md` is fixed across projects.
`PROJECT.md` accumulates what is specific to this one.

Update this file when a new project-specific constraint, environment
detail, or convention is discovered during work.

Do not duplicate:
- verified architecture → `docs/`;
- current work → `plan.md`;
- unresolved problems → `issues.md`.

## Template

- Initialized from (source, version, or commit): explicit-agent 0.1.0 (964b9d9c376f)
- Last synced to (source, version, or commit): explicit-agent 0.1.0 (964b9d9c376f)
- Last applied migration id (see the template's `migrations.json`): 0

## Purpose

个人使用的任务管理系统：分层任务（长期 / 本周 / 今日）、按日期自动顺延、research 笔记、DeepSeek 快速问答与会话管理。以 Rainmeter 皮肤的形式常驻桌面，所有内容由 git 管理。

## Environment

- Platform / hardware: Windows 11 (win32 10.0.26200)，x64。
- Toolchain: MSVC 14.44（`D:\Microsoft Visual Studio\2022\Community`）、CMake（`C:\Program Files\CMake\bin`）、git 2.46.2、Python 3.11.7、uv 0.12.10；未安装 Node.js。
- Rainmeter: 程序位于 `C:\Program Files\Rainmeter`，自带 `RunCommand.dll` 与 `InputText.dll`（InputText 1.7.0.0 的输入框在本机上创建后立即关闭，皮肤改用 `todo.exe prompt`）；皮肤目录（`Rainmeter.ini` 中的 `SkinPath`）位于用户目录下的 OneDrive 文档文件夹（`%USERPROFILE%\OneDrive\文档\Rainmeter\Skins\`），在 OneDrive 同步范围内。
- Shared with (other users or projects on the same account or machine): 单用户个人机器。
- 时区：系统时区为美东（Windows 时区 id `Eastern Standard Time`，夏令时 UTC−4，冬令时 UTC−5）。
- Constraints imposed by the environment: 用户目录和皮肤目录的路径中包含中文字符。

## Credentials

Services that require a scoped identity in this project (git remotes,
package registries, model hubs, cloud APIs).

| Service | Scope | Source | Notes |
|---|---|---|---|
| git（代码仓库与内容仓库） | 本机 | 全局 git 配置中的现有身份 | 内容仓库没有远程 |
| GitHub（代码仓库的远程 `origin` = `https://github.com/luo-jingw/wallpaper.git`，公开） | 使用者本人的 GitHub 账号 | 全局 git 配置的 Git Credential Manager（`credential.helper=manager`） | 只推送代码仓库；内容仓库与 `ds_token` 不推送。公开的 `main` 由当前文件生成的、没有父提交的提交组成，不含本地历史 |
| DeepSeek API | 本项目 | 代码仓库根目录下的 `ds_token` 文件 | `ds_token` 必须被 `.gitignore` 忽略 |

Do not assume a global or default identity applies.
See `AGENTS.md` → Credentials and Environment Isolation.

Do not record token values or other secrets here.

## Project-Specific Constraints

Rules that apply only to this project, in addition to `AGENTS.md`.

- 代码仓库为 `e:\python_proj\ToDo`，内容仓库为 `e:\python_proj\ToDo-data`，两者是相互独立的 git 仓库。
- 内容仓库中的文件只由 `todo.exe` 写入，或由使用者手动编辑；Rainmeter 皮肤不直接写内容文件。
- 内容仓库中的每次修改对应一个 commit。
- 所有文件系统路径和进程参数使用宽字符 API（`wmain`、`std::filesystem::path`、`CreateProcessW`），以支持中文路径。
- `todo.exe` 运行时不得出现可见窗口；它由 `RunCommand` 以 `State=Hide` 启动。
- 时间只有一个来源：系统本地时间（`GetLocalTime`、Lua `os.date`、git 提交时间）。不在程序中写死时区；系统时区改变时全部随之改变。发给模型的当前时间注明系统时区（`DateTime::local_zone_label()`），带 UTC 偏移的外部时间（如网页搜索结果）换算成本地日期（`DateTime::local_date_of()`）。
- DeepSeek Harness（[deepseek-ai/deepseek-harness](https://github.com/deepseek-ai/deepseek-harness)）只作为设计参考：一次性提问模式和会话持久化。不作为运行时依赖。

## Onboarding

What a new contributor must read or know before making the first change.
