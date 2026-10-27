# Measurements

环境：Windows 11（10.0.26200），MSVC 14.44 Release 构建，Rainmeter 4.5.28，git 2.46，数据目录路径含中文。日期为 2026-10-19（ISO 周 2026-W43）。

## Command Latency

| 操作 | 观测值 |
|---|---|
| 只读命令（`list`） | 约 8 ms |
| 修改类命令（含 git 提交） | 约 300 ms |
| 首次运行（`git init` + 顺延状态提交） | 约 700 ms |
| DeepSeek `deepseek-flash`，thinking disabled，短问答 | 666–1210 ms |
| DeepSeek 长回答（496 个输出 token） | 4287 ms |
| 对话皮肤 `Send` → 内容仓库出现提交 | 1553 ms（其中接口 1210 ms） |

## Chat Requests

| 场景 | request_messages | system_chars | tokens_in |
|---|---|---|---|
| `ask`，无任务上下文 | 2 | 166 | 51–67 |
| 同一会话第二轮 | 4 | 166 | 315 |
| 同一会话第三轮 | 6 | 166 | — |
| `--with-tasks`，smoke 测试数据 | 2 | 382 | — |
| `--with-tasks`，皮肤测试数据（13 个今日任务） | 2 | — | 552 |

assistant skill（2026-10-20，真实调用）：读取 research 笔记并 `add_task`。4 轮 completion，3 次工具（`list_dir`、`read_file`、`add_task`），合计 5225 / 229 tokens，4368 ms。无 skill 的 `ask`：`CONFIG tools=no`，请求不含 tools 字段。

`deepseek_probe`（`deepseek-flash`，thinking 关闭，2026-10-20）：

| 请求 | latency_ms | tokens_in | tokens_out | 结果 |
|---|---|---|---|---|
| 带 1 个工具，问今日与本周任务数 | 1467 | 300 | 67 | 同一条回复里 2 个并行 `tool_calls`，`content` 为空 |
| 回传 2 条 `tool` 结果 | 1163 | 395 | 18 | 最终回答使用了两个结果 |
| `json_object`，提示含示例结构 | 931 | 70 | 42 | 输出可解析，2 个时间块 |

## Validation Scenarios

`python scripts\smoke_test.py <scenario>` 在临时目录（路径含中文）中运行，场景：`tasks`, `places`, `rollover`, `git`, `research`, `tools`, `chat`, `skills`, `plan`（`chat`、`skills`、`plan` 发起真实 API 调用）。

| 场景 | 观测 |
|---|---|
| tasks | 解析 → 序列化 往返，校验和不变（`228d7222611fd57c`） |
| rollover | 跨日、跳过多日、跨周顺延的 `ROLLOVER` 记录与文件内容；同一天重复执行校验和一致 |
| git | 提交数与修改类命令数一致；无修改时 `sync` 输出 `COMMIT none` |
| research | 中文标题生成文件与 `NOTE` 记录，重名追加 `-2` |
| chat | `USAGE` 记录；续聊消息数递增；非法 ID 与缺失 API Key 均输出 `ERR` 且不发送请求 |
| plan | 日计划与周计划的时间块、按块统计的 remote ↔ 校园切换次数与通勤分钟数；`plan apply 1,3` 后的任务文件与计划状态；`plan_probe` 对 6 类非法回复输出 `ERR`，数据目录校验和不变 |

## Planning

2026-10-20，`deepseek-flash`，thinking 关闭，6 个日任务、2 个周任务、1 个长期任务：

| 计划 | 时间块 | remote ↔ 校园切换 | 通勤块 / 分钟 | 延迟 ms | tokens in / out | 请求字符数 |
|---|---|---|---|---|---|---|
| day（第 1 次） | 12 | 2 | 2 / 75 | 3011 | 1127 / 634 | 3868 |
| day（第 2 次） | 11 | 1 | 1 / 60 | 2863 | 1127 / 565 | 3868 |
| week（周二至周五） | 24 | 3 | 6 / 320 | 3991 | 1161 / 987 | 3946 |

## Desktop Skin

通过 `scripts\capture_window.ps1`（PrintWindow）截图观测，交互由 `Rainmeter.exe !CommandMeasure` 触发。合并皮肤 `ToDo\Main` 在沙盒内容仓库上观测：

- 任务视图（style C）：状态与地点 Shape、估时、删除线、进度条；`OnAddClick` 加 `send_text.ps1` 添加中文任务，`OnPlaceClick` 设置地点，均产生提交。
- 对话：选择会话后显示 `TOOL` 行；追问后对话区展开到 240 像素并滚动；底部 `2 tools | 4.4k / 128 tokens | 3.0s`；折叠后窗口高度从 723 降到 340。
- Plan 标签页：日计划 12 块；取消勾选一项后 `apply 1` 只应用勾选项，状态变为 applied；周计划 3 页，`why` 显示 rationale。
- 样式：点击样式标记后 `Style` 依次写为 a、b、c，刷新后保持；三套配色均正确渲染。
- 窗口宽 420；高度随内容变化（121 到 746）。

吉祥物 `ToDo\Mascot`（Rainmeter 进程 CPU，10 s 平均，单核百分比）：无吉祥物 6.6%；每帧对图层设置 TransformationMatrix（25 fps）74–99%；预旋转帧加 `!MoveMeter`（25 fps）4.2%。

换帧方式对比（2026-10-22，临时测试皮肤，10 个动画图层用吉祥物真实帧，Update=15，每次更新每层都换帧；12 s 平均，3 轮交替，单核百分比；不含皮肤时 2.6–4.6%）：

| 换帧方式 | meter 数 | CPU |
|---|---|---|
| 静态，不换帧 | 12 | 8.1 / 8.5 / 9.2 |
| 静态，不换帧，另有 1735 个隐藏帧 | 1747 | 5.6 / 10.9 / 8.9 |
| 每帧一个 meter，显示 / 隐藏 | 1737 | 6.3 / 8.7 / 8.1 |
| 每层一个 meter，`!SetOption ImageName` | 12 | 36.6 / 28.8 / 33.3 |
| 每层一张 sprite sheet，`!SetOption ImageCrop` | 12 | 25.8 / 30.2 / 23.0 |
| 每层一张 sprite sheet 放在容器里，改 X / Y 滚动 | 22 | 28.1 / 25.9 / 28.6 |

显示 / 隐藏与静态相当；隐藏 meter 的数量在噪声范围内没有可见影响；三种改图片选项的方式都高出 3–4 倍。皮肤在 Update=15 下即使画面不变也有约 5 个百分点的固定开销。

吉祥物的 meter 更新与睡眠（2026-10-22，单核百分比，15 s 平均，4 轮交替取中位数）：醒着、每个 meter 每帧更新 12.9%；醒着、`DefaultUpdateDivider=-1` 11.8%；睡着（Lua 不再换帧）、每个 meter 每帧更新 11.3%；睡着、`DefaultUpdateDivider=-1` 4.3%；关闭吉祥物 3.3%。`DefaultUpdateDivider=-1` 下连续截图每秒约 46 个不同画面，与每帧更新时相同（约 41 个），即皮肤仍每帧重绘。`!SetOption Rainmeter Update` 在运行时不生效（改为 250 后 CPU 不变）；重载一次吉祥物皮肤约耗 2.1 s CPU、约 1.1 s。从睡眠中 `Pat()` 到醒着的第一帧 2 ms。

吉祥物拖动（2026-10-22，`mouse_event` 按下后 `SetCursorPos` 每 16 ms 移动一步、共 400 px，逐帧日志）：拖动过程中 `#CURRENTCONFIGX#` 与系统窗口矩形逐帧一致。约 1000 px/s 拖动时上半身约 1.9°、头约 5°、尾巴约 −15°；松手后长发物理位移最大约 155 源像素（显示约 59），约 2 s 内回摆停住。松手不触发点击动作。

旧的 `ToDo\Tasks` 与 `ToDo\Chat` 皮肤的观测：

- 任务皮肤：中文显示、状态切换、右键状态、标签页切换、13 个任务分页。
- 对话皮肤：列表折叠与展开、选择会话、多段回答换行、自动定位到最新问题、向上和向下滚动边界、等待状态、任务上下文开关、`CurrentChat` 与 `WithTasks` 写入 `ui_state.inc`。
- 运行期间没有出现终端窗口；`prompt` 进程的控制台窗口为隐藏状态。

## Text Input

显示器：`DISPLAY1` 2560×1600（主屏），`DISPLAY2` 1920×1080，位于 x=2560 起，均为物理像素。皮肤位于副屏。

| 观测 | 结果 |
|---|---|
| 皮肤 bang → 输入窗口出现 | 68–70 ms |
| 皮肤物理位置（Rainmeter 记录的坐标） | Tasks (2954,168)–(3429,633)（记录为 2363,134）；Chat (3429,166)–(3954,754)（记录为 2743,133） |
| 输入窗口默认位置（物理） | Tasks (2954,218)–(3429,303)；Chat (3429,641)–(3954,726)：左边与皮肤对齐，偏移 = `PromptOffsetY` × 1.25 |
| 拖动 +100,+60 物理像素后按 Esc | `POSITION 80 88` 写入 `TasksPromptX/Y`；再次打开位于 (3054,278) |
| 输入窗口是否为前台（由后台进程触发） | 是 |
| `send_text.ps1` 输入中文后按 Enter | 任务写入并提交；提问约 2.4 s 后会话文件写入（接口 1232 ms） |
| 按 Esc | 输出 `CANCEL`，不写入任何内容 |

Rainmeter InputText 1.7.0.0 在本机上的观测：输入框创建后立即关闭，并以 `DefaultValue` 执行 `Command1`；重启 Rainmeter 后，从 ToDo 皮肤触发时不出现窗口。

未经自动化观测：真实鼠标点击；通过输入法组合输入中文（自动化发送的是 Unicode 按键事件）。

## Path Policy and Confirm

`tools_probe`（2026-10-20，沙盒目录名含中文）：

| 请求 | 判定 |
|---|---|
| 内容仓库相对路径 `tasks/note.md` 读/写 | 允许，不确认 |
| `Tasks/note.md`（大小写不同） | 允许，解析到同一文件 |
| `../outside.md` 与允许目录的 `../outside.md` | 拒绝 |
| 允许目录内绝对路径 读 / 写 | 允许；写需要确认 |
| `.git/config` | 拒绝 |
| `C:/Windows/notepad.exe` 读 | 拒绝 |
| 目录符号链接 | 本机无创建符号链接权限，跳过 |

确认窗口（允许目录中的 `write_file`，`send_text.ps1`）：Enter 写入，校验和改变；Esc 拒绝，校验和不变。内容仓库内的写入不弹出确认窗口。
