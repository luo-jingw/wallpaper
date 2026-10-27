# Architecture

## Components

系统由三部分组成：

- `todo.exe`：C++20 控制台程序，读写内容仓库并调用 DeepSeek。所有内容修改都经过它。
- Rainmeter 皮肤 `ToDo\Main`：通过 `RunCommand` 插件以 `State=Hide` 启动 `todo.exe`，解析其标准输出并渲染。皮肤不写内容文件，因此不会出现终端窗口。
- 内容仓库 `E:\python_proj\ToDo-data`：独立的 git 仓库，保存任务、research 笔记、会话、skill、计划与地点 / 日程说明，只在本地。

## Modules

| 模块 | 文件 | 职责 |
|---|---|---|
| Foundation | `src/util/utf8.*`, `src/util/date.*`, `src/util/text.*`, `src/util/todo_error.h`, `src/config/app_config.*` | UTF-8 与宽字符转换、日期与 ISO 周、文本工具、读取 `todo.ini` 与 API Key |
| Task Domain | `src/domain/task.h`, `src/domain/task_status.*`, `src/domain/task_level.*`, `src/domain/task_place.*`, `src/domain/task_text.*` | 任务、状态、层级、地点；给模型用的一行描述 |
| Task Storage | `src/storage/task_file.*`, `src/storage/task_repository.*` | 任务 Markdown 的解析与序列化；按层级和日期定位文件；分配 ID |
| Rollover | `src/rollover/rollover_service.*` | 跨日、跨周顺延未完成任务 |
| Process | `src/process/process_runner.*` | 以 `CREATE_NO_WINDOW` 启动子进程并捕获输出 |
| Version Control | `src/vcs/git_client.*` | 通过 `git.exe` 初始化内容仓库并提交 |
| Research | `src/research/research_store.*` | research 笔记的创建与列出 |
| AI Transport | `src/ai/https_client.*`, `src/ai/deepseek_client.*`, `src/ai/web_search_client.*` | WinHTTP 上的 HTTPS POST；DeepSeek chat completions 请求与响应；DeepSeek 网页搜索 |
| Chat | `src/chat/chat_session.h`, `src/chat/chat_store.*`, `src/chat/chat_service.*` | 会话文件读写（`ChatStore` 按目录区分 `chats/` 与 `companion/`）；按 `ChatProfile`（提示词、工具、上下文轮数、是否附任务）发送；每条用户消息最多 8 轮 completion，中间轮可带 tools |
| Companion | `src/companion/companion_service.*`, `rainmeter/ToDo/Companion/Companion.ini`, `rainmeter/ToDo/@Resources/Scripts/companion_view.lua` | 桌宠的只读对话：当天会话选择、固定人设与只读工具清单；桌宠旁的对话气泡 |
| Skills | `src/skills/skill_store.*`, `src/skills/default_skills.*` | 读取 `skills/*.md`；目录为空时写入 assistant / planner / replan / research。`replan` 先读 `plans/<period>.md` 与 profile，只改用户要求的时间块，再调用一次 `propose_plan` |
| Prompt | `src/prompt/prompt_window.*`, `src/prompt/window_support.*`, `src/prompt/confirm_window.*` | 置顶的原生单行输入窗口；写入允许目录前的确认窗口（Enter 允许，Esc 拒绝） |
| Planning | `src/planning/*` | 计划周期（日 / ISO 周）；时间块与改动的校验和行格式；模型 JSON 回复解析；`plans/` 读写；示例 profile；`Planner` 发起 json_object 请求；`PlanApplier` 通过 add_task / set_task_place 工具应用选中的改动 |
| Tools | `src/tools/path_policy.*`, `src/tools/tool*.{h,cpp}`, `src/tools/task_tools.*`, `src/tools/file_tools.*`, `src/tools/search_tools.*` | 路径判定；任务与文件工具；本地与网页搜索；工具注册表 |
| CLI | `src/main.cpp`, `src/cli/command{,_parser,_runner,_context}.*`, `src/cli/{task,research,conversation,companion,plan}_commands.*`, `src/cli/prompt_command.*`, `src/cli/conversation_tools.*`, `src/cli/skin_output.*` | 参数解析；`CommandRunner` 按动词分发到各领域的处理器，处理器共享 `CommandContext`（配置、日期、输出、存储、git、提交与参数检查）；`ConversationTools` 是唯一构建对话工具注册表的地方（完整 / 只读，只读集合与 `CompanionService::read_only_tools()` 核对）；皮肤输出协议 |
| Desktop Skin | `rainmeter/ToDo/Main/Main.ini`（由 `scripts/gen_main_ini.py` 生成）, `rainmeter/ToDo/@Resources/` | 合并皮肤；各视图脚本经 `todo_protocol.lua`（`dofile` 载入）解析 todo.exe 输出并运行命令：`task_view.lua`（头部、标签页、任务行）、`plan_view.lua`（Plan 标签页）、`chat_view.lua`（提问栏、对话、会话列表、消息行、背景）；`style-{a,b,c}.inc` 三套配色 |
| Mascot | `rainmeter/ToDo/Mascot/Mascot.ini`, `rainmeter/ToDo/@Resources/Scripts/mascot_view.lua`, `rainmeter/ToDo/@Resources/Mascot/`, `design/mascot/{split_layers,export_skin_assets}.py`, `plugins/cursor_pos/`, `plugins/desktop_state/` | 分层吉祥物动画：眨眼、摸头、吃饭、写字、长发 / 尾巴 / 呆毛 / 耳鳍摆动、拖动时甩动、看向 ToDo 窗口、空闲时睡觉；Main 在生成计划时调用 `StartWriting()` / `StopWriting()`，收到回答时调用 `Eat()` |
| Install / Validation | `scripts/install_skin.ps1`, `scripts/smoke_test.py`, `probes/plan_probe.cpp`, `scripts/capture_window.ps1`, `scripts/send_text.ps1` | 安装皮肤；沙盒场景验证；按窗口标题截图；向指定进程的前台窗口发送 Unicode 按键 |

第三方依赖：`third_party/nlohmann/json.hpp`（nlohmann/json 3.11.3 单头文件，SHA256 记录在 `CMakeLists.txt`）。系统库：`winhttp`、`user32`、`gdi32`。

## State Ownership

| 状态 | 所有者 | 存储位置 |
|---|---|---|
| 日 / 周 / 长期任务 | `TaskRepository` | `tasks/days/YYYY-MM-DD.md`, `tasks/weeks/YYYY-Www.md`, `tasks/long_term.md` |
| 最近一次顺延日期 | `RolloverService` | `tasks/rollover_state.md` |
| research 笔记 | `ResearchStore` | `research/YYYY-MM-DD-<slug>.md` |
| 会话 | `ChatStore` | `chats/<chat_id>.md` |
| skill | 使用者；`SkillStore` 只读，缺失时写入默认文件 | `skills/<name>.md` |
| 计划 | `PlanStore` | `plans/YYYY-MM-DD.md`, `plans/YYYY-Www.md` |
| 地点与通勤、每周固定安排 | 使用者；`PlanProfile` 只在文件不存在时写入示例 | `profile/places.md`, `profile/schedule.md` |
| 内容仓库历史 | `GitClient` | `ToDo-data/.git` |
| 应用配置 | `AppConfig` | `ToDo/todo.ini` |
| DeepSeek API Key | `AppConfig`（只读） | `ToDo/ds_token`，被 `.gitignore` 排除 |
| 皮肤路径 | `install_skin.ps1` | 安装目录下 `@Resources/paths.inc`（UTF-16LE） |
| 皮肤界面状态 | 皮肤 Lua 脚本 | 安装目录下 `@Resources/ui_state.inc`：`View`（plan / tasks；旧值 day / week / long 读作 tasks）, `TaskScope`（day / week / long）, `CurrentChat`, `WithTasks`, `Skill`, `ChatOpen`, `Style`（a / b / c）, `TasksPromptX/Y`, `ChatPromptX/Y`；重新安装时保留，缺少的键按默认值补上 |

## Main Flow

```text
wmain(argv)
  → CommandParser::parse → Command
  → AppConfig::load(--config)
  → 若命令不是 config 或 prompt：
      GitClient::ensure_repository（git init -b main，写入 .gitattributes: * text=auto eol=lf）
      RolloverService::ensure(today) → ROLLOVER 记录；performed 时提交
  → CommandRunner::run
      修改类命令执行后 GitClient::commit_all(<命令摘要>) → COMMIT 记录
      对话命令：ChatService::send → DeepSeekClient::complete → HttpsClient::post_json
  → 退出码：0 成功；1 为 TodoError；2 为其它异常
```

## Rollover

- `rollover_state.md` 记录最近一次顺延日期 `L`。不存在时写入今天，不移动任务。
- `L < today` 时，`L` 日文件中状态为 todo、doing、blocked 的任务复制到今日文件，保留 `id` 与 `created`，`rolled + 1`；原条目标记为 `[>]`。中间跳过的日子不产生文件。
- `L` 与今天不在同一 ISO 周时，对周任务执行同样的操作。长期任务不顺延。
- 同一天重复执行不改变任何文件。

## Chat

- `ask` 新建会话（标题取问题前 30 个码点）并发送；`chat send` 在已有会话中续聊，请求包含全部历史轮次。
- 系统提示包含今天的日期和 ISO 周；`--with-tasks` 时追加今日、本周、长期任务列表。会话带 skill 时改用该 skill 的系统提示，并发送其工具清单。
- 每条用户消息最多 8 次 completion；最后一次不带 tools。工具调用写在回答前的 `## tool` 段，并输出 `TOOL` 记录。
- 收到回答后才写入会话文件，失败时会话文件不变。
- API Key 只在对话命令中读取；缺失或为空时输出 `ERR`，不发送请求。
- 请求体：`model`、`stream: false`、`thinking: {type: enabled|disabled}`、`messages`；有 skill 时含 `tools`。
- 文件工具：内容仓库可读写；`allowed_dirs` 内可读，写入前弹出确认窗口；其它路径拒绝。`.git` 不可访问。

## Planning

- `plan day|week [--date] [--chat]`：输入为当前时间与星期、当日 / 本周 / 长期任务（`TaskText::describe`，含地点、估时、最新备注）、`profile/places.md` 与 `profile/schedule.md` 原文、可选的会话最近 12 轮（每轮最多 800 码点）、`planner` skill 的系统提示、`PlanFormat` 的 JSON 示例与规则（不早于当前时间；会话中的说明优先；写明某一天的任务只排在那一天）；请求使用 `response_format: json_object`，不带 tools。
- 回复先由 `PlanParser` 完整校验（时间格式、kind、place、task_id 格式、改动 op 与参数），任何不符都输出 `ERR`，不写任何文件、不提交。校验通过后写入 `plans/<period>.md`（`status=proposed`；同一周期覆盖旧计划，旧版本留在 git 历史中）并提交。
- `plan apply <period> <all|n,...>`：只接受 `proposed` 计划；先检查编号与任务都存在，再依次调用 `AddTaskTool` / `SetTaskPlaceTool`。任务文件以计划周期的锚定日期为准：日计划为该日；周计划为今天（今天在该周内时）或该周周一。状态改为 `applied` 并记录 `applied=` 编号，一次提交。
- `plan discard <period>` 只接受 `proposed` 计划。
- 对话中的 `propose_plan` 工具由模型直接给出时间块与改动，经 `PlanParser` 校验后保存为 `proposed`，在对话命令结束时随会话一起提交。
- 对话系统提示包含当前日期、时间、星期与 ISO 周。
- 程序不检查时间块是否可行（例如通勤时长），见 `issues.md`。

## Desktop Skin

- `.ini` 与 `.lua` 文件只含 ASCII；中文只来自 `todo.exe` 的 UTF-8 输出（`OutputType=UTF8`）。
- 单一皮肤 `ToDo\Main`，宽 420。布局自上而下：头部（日期、子标题、样式标记、Plan / Tasks 分段控件，默认 Plan）→ 进度条 → Plan 标签页或 Tasks 标签页（day / week / long 小标签与 `+ add`，下接任务行）→ 提问栏 → 对话区 → 消息行。`task_view.lua` 或 `plan_view.lua` 排好上半部分后调用 `chat_view.lua` 的 `SetTop(y)`，由后者放置提问栏和对话区并设置背景高度；其它视图的消息通过 `ShowMessage` 交给 `chat_view.lua`。
- 状态与地点标记用 Shape 绘制（todo 空心圆、doing 半圆、done 实心点、blocked / dropped 带斜线的圆；L / C / R 彩色圆点），不依赖非 ASCII 字符。
- 任务行：左键状态循环 todo → doing → done → todo，右键 blocked → dropped → todo；点击地点圆点循环 any → lab → class → remote；点击标题添加备注；右侧显示估时（无估时且非今天创建时显示创建日期）与顺延次数 `rN`；每页 10 行，分页；每 60 秒刷新。点击日期执行 `sync` 并刷新。
- Plan 标签页：`day` / `week` 切换后运行 `plan show`；时间轴每页 12 块（时间、按地点着色的节点、标题、时长；通勤为虚线圆和斜体）；子标题显示通勤总时长与 remote ↔ 校园切换次数；改动默认全选，点击切换，`apply N` 运行 `plan apply`，`discard` 运行 `plan discard`，`plan` / `re-plan` 运行 `plan day|week`，有当前会话时附 `--chat <CurrentChat>`；改动行用 `CHANGE` 记录中的任务标题显示；`why` 显示 rationale。关联任务的时间块左侧显示该任务的状态标记（圆圈颜色为地点），点击按 todo → doing → done 推进；底部 NOT IN PLAN 列出计划未引用的未完成日 / 周任务（最多 6 行，可同样点击），没有计划时标题为 OPEN TASKS。每 60 秒及对话使用工具后刷新。对话中 `propose_plan` 成功后，皮肤切到 Plan 标签并显示对应的日或周计划。
- 提问栏：skill 标记在 `no skill` 与已有 skill 之间循环，作用于新会话，当前有会话时开始新会话；`tasks` 切换 `--with-tasks`；三横线图标打开会话列表（每页 8 行，最新在前）。有回答时对话区展开，高度最多 240 像素，可滚动；`TOOL` 记录显示在对应回答之前（等宽、按状态着色）；底部显示工具数、token、延迟，`new` 新建会话，`^` / `v` 折叠或展开（`ChatOpen`）。对话中执行过工具后刷新任务列表。
- 样式：`ui_state.inc` 的 `Style` 决定包含 `style-a.inc`、`style-b.inc` 或 `style-c.inc`（颜色、字号、行高、内边距）；点击头部样式标记写入下一个值并刷新皮肤。
- 文本输入：皮肤用 RunCommand 运行 `todo.exe prompt`，传入自身窗口标题（`#CURRENTPATH##CURRENTFILE#`），以及保存的偏移；没有保存过时，任务输入的默认偏移为 (0, 任务区顶部)，提问为 (0, 提问栏位置)。对话请求同样传入 `--anchor`，写入确认窗口盖在皮肤上。输入窗口盖在皮肤上，可以拖动，位置会被记住。`FinishAction` 调用 `OnPromptFinished()` 读取 `INPUT` 或 `CANCEL`。打开期间不会重复打开。不使用 Rainmeter 的 InputText 插件：在本机上，它的输入框创建后立即关闭。
- 吉祥物 `ToDo\Mascot`：独立皮肤，Update=15（约 64 fps）。素材由 `design/mascot/split_layers.py`（从生成图拆出补丁层与部件层）和 `export_skin_assets.py`（裁剪缩放到 166 px 宽、预渲染每个部件的各个角度、生成 `layout.inc` 与 `frames.inc`）产出，每一帧是一个 Image meter。运行时只显示 / 隐藏帧，头部各层用 `!MoveMeter` 整像素平移，只有耳鳍和呆毛带亚像素平移矩阵。meter 不自行更新（`DefaultUpdateDivider=-1`，只有三个 measure 每帧更新）：`!ShowMeter` / `!HideMeter` / `!MoveMeter` 立即生效，经 `!SetOption` 修改的 meter（眼珠位置、平移矩阵）随后用 `!UpdateMeter` 更新；皮肤仍每帧重绘。不使用旋转矩阵（每个带旋转的图片在 25 fps 下约占单核 20%）。点击头部为摸头，点击身体为喂饭，双击显示 / 隐藏 ToDo 窗口。双击的第一下也会触发单击，所以单击动作等待 0.45 s（Windows 默认双击间隔 0.5 s）再执行；双击时取消等待中的单击，并忽略随后 0.45 s 内的单击。Main 调用的 `Eat()` / `Pat()` 立即执行。
- 吉祥物的输入来自两个插件：`CursorPos`（光标位置与高精度时钟，字符串 `X,Y,MS`）和 `DesktopState`（字符串 `VISIBLE,LEFT,TOP,RIGHT,BOTTOM,IDLE_MS,FULLSCREEN`：被监视窗口的可见性与矩形、距上次键鼠输入的毫秒数、前台窗口是否无标题栏且覆盖整个显示器；任务栏自动隐藏时最大化窗口也覆盖整个显示器，但带标题栏）。被监视窗口由 `!CommandMeasure MeasureDesktop "Watch <窗口标题>"` 设置，Mascot.ini 在加载时监视 `ToDo\Main`。
- 拖动：Lua 由 `#CURRENTCONFIGX/Y#`（拖动过程中逐帧更新）求窗口速度（平滑后），把速度变化量反向加到上半身、头、长发、呆毛、尾巴的弹簧速度上，竖直方向的变化加到耳鳍上；拖动中各部件的弹簧目标按速度向后偏移（上限 1500 px/s）。长发显示值经 `60·tanh(x/60)` 平滑饱和到预渲染的 ±60 源像素范围内。点击区域不显示提示文字。
- ToDo 窗口可见（由 `DesktopState` 观察，任何打开方式都算）且她醒着时，基础姿势为拿着本子写字：吃饭、摸头仍可插入，结束后回到写字；随机动作不再抽到写笔记；眨眼、晃动、重心与歪头事件照常；每 6–14 s 抬头 1.2–2.5 s（眼睛跟随光标或 ToDo 窗口，笔停下）。入睡时放下本子。
- 对话气泡 `ToDo\Companion`：点击尾巴或任一耳鳍（该部件随之一甩）或桌宠右键菜单，经 `ToggleCompanion()` 打开或关闭；0.45 s 内的第二次点击只甩动不切换。尾巴与耳鳍的点击区域是静止帧轮廓外扩 3 px 的椭圆（`layout.inc` 的 `TailHit*`、`FinLHit*`、`FinRHit*`，由 `export_skin_assets.py` 生成），位于身体与头部点击区域之下，重叠处仍为喂饭 / 摸头。切换前 `!UpdateMeasure MeasureCompanionWindow` 读取气泡是否打开；打开时 `!Move` 到桌宠左侧（屏幕左侧放不下时放右侧），与头部齐平。气泡 Update=1000，只在点击与 `todo.exe companion` 返回时重排；文字底部对齐，滚轮查看更早内容，内容超出时右侧显示滚动条。输入是 `todo.exe prompt --dock-below`，贴在气泡下方并随之移动，配色取当前风格；气泡空闲时自动打开，发送后关闭并显示 "Thinking..."，回答到达后重新打开；Esc 关闭后由标题栏的 ask 重新打开。收到回答时调用桌宠 `Pat()`。气泡中去掉残留的 `**` 与反引号。
- 视线：ToDo 窗口可见、且光标不在该窗口上也不在她眼睛 220 皮肤像素以内时，眼珠看向 ToDo 窗口中心；否则跟随光标。
- 睡觉：90 s 无键鼠输入或前台为全屏程序时开始入睡（闭眼、低头、耳鳍下垂），4 s 后停住：Lua 每帧只读取输入并检查是否该醒，不再换帧。输入、拖动或动作请求时在下一帧醒来，以眨眼和耳鳍一抖开始。
- 所有皮肤交互都是 Lua 函数，可用 `Rainmeter.exe !CommandMeasure <MeasureTasks|MeasurePlan|MeasureChat> "<函数>" "ToDo\Main"` 触发；`MeasureChat` 的 `Send(text)` 与输入框回车走同一条路径。
