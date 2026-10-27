# Plan

Plan Status: completed

# Problem

## Current

- 桌面上有两个独立皮肤：`ToDo\Tasks`（日 / 周 / 长期任务）和 `ToDo\Chat`（DeepSeek 对话）。文本输入由 `todo.exe prompt` 负责。
- 对话只能读到可选的任务列表（`--with-tasks`），不能读文件、改文件或操作任务。
- 任务没有地点和时长信息。使用者的日常在三个地点之间切换：实验室、上课、remote。实验室与上课地点相邻，remote 与它们之间通勤约 1 小时。
- 皮肤是纯文字界面：状态标记是 `[ ]` 这样的字符，没有图标、悬停效果或进度信息。
- DeepSeek `deepseek-flash` 支持 OpenAI 格式的 `tools` / `tool_calls`，以及 `response_format: {"type": "json_object"}`；不支持 `json_schema`。

## Problem

1. 任务和对话分在两个窗口，占用桌面空间，也不能边看任务边提问。
2. AI 无法读取指定文件，也无法直接帮忙记录任务或修改笔记，只能复制粘贴。
3. 排日程时需要考虑地点和通勤（remote ↔ 学校往返约 2 小时），目前没有地方记录这些信息，AI 也无从参考。
4. 界面信息密度低，状态不易一眼看清。

## Goal

1. 一个合并的皮肤：上方是任务列表（Today / Week / Long / Plan 标签页），下方固定一条提问栏；有回答时对话区向下展开，可以折叠。
2. 对话可以选择 skill。skill 是内容仓库 `skills/` 下的 Markdown 文件，包含系统提示和允许使用的工具清单。可用工具：读写任务、列出目录、读文件、写文件、替换文件中的文本。
3. 文件权限：内容仓库可读写；`todo.ini` 的 `allowed_dirs` 中列出的目录可读，写入前弹出确认窗口。其它路径一律拒绝。每次工具调用都记录在会话文件中，并在皮肤上显示。
4. 任务可标注地点（`lab` / `class` / `remote` / `any`）和预计时长（分钟）。
5. `plan day|week` 请 DeepSeek 生成计划：输入为任务、地点与通勤说明、每周固定安排，输出按时间块排好的计划和建议的任务改动。计划写入 `plans/` 并在 Plan 标签页显示；建议的改动经确认后才写入任务。不做实时提醒。
6. 合并皮肤的布局按选定的 `variant-c` 实现；`ui_state.inc` 的 `Style` 可在 `a` / `b` / `c` 之间切换（默认 `c`），分别对应三版效果图的配色与间距。布局不变。

# Structure

## Modules

| 模块 | 位置 | 职责 |
|---|---|---|
| Task Domain / Storage | `src/domain/`, `src/storage/` | 新增任务字段 `place`、`estimate_minutes` |
| AI Transport | `src/ai/` | 请求支持 `tools` 与 `response_format`；响应解析 `tool_calls` |
| Access Policy | `src/tools/path_policy.*` | 把路径解析到内容仓库或允许目录，判定读、写是否需要确认 |
| Tools | `src/tools/` | 工具定义与执行：任务工具、文件工具；工具注册表 |
| Confirm | `src/prompt/confirm_window.*` | 原生确认窗口：Enter 允许，Esc 拒绝 |
| Skills | `src/skills/` | 读取 `skills/*.md`；内容仓库为空时写入默认 skill |
| Chat | `src/chat/` | 带工具调用的对话循环；会话记录 skill 名称和工具调用 |
| Planning | `src/planning/` | 构造规划请求、校验 JSON 计划、写入和读取 `plans/`、应用选中的改动 |
| CLI | `src/cli/`, `src/main.cpp` | 新命令与新记录类型 |
| Desktop Skin | `rainmeter/ToDo/Main/`, `rainmeter/ToDo/@Resources/` | 合并后的皮肤；任务、对话、计划三个 Lua 视图 |
| Design | `design/mockups/` | 外观效果图（HTML 与截图） |

## Responsibilities

- `PathPolicy` 是判定文件访问的唯一位置。文件工具先调用它，再进行读写。
- 工具执行只在 `todo.exe` 内进行。皮肤只显示 `TOOL` 记录。
- 内容仓库的所有修改（包括工具写入和计划应用）都在命令结束时由 `GitClient` 提交一次。允许目录中的文件不受 git 管理，写入前必须确认。
- `profile/places.md` 与 `profile/schedule.md` 是自由格式的 Markdown，由使用者编辑。程序不解析它们，原文交给规划请求。

## State Ownership

| 状态 | 所有者 | 存储位置 |
|---|---|---|
| 任务地点、时长 | `TaskRepository` | 任务行元数据 `place=`、`est=` |
| skill 定义 | 使用者（手动编辑）；`SkillStore` 只读，并在缺失时写入默认文件 | `ToDo-data/skills/<name>.md` |
| 会话使用的 skill、工具调用记录 | `ChatStore` | 会话文件头部 `skill=`；`## tool` 段落 |
| 地点与通勤说明、每周固定安排 | 使用者（手动编辑） | `ToDo-data/profile/places.md`、`profile/schedule.md` |
| 计划与建议改动 | `PlanStore` | `ToDo-data/plans/YYYY-MM-DD.md`、`plans/YYYY-Www.md` |
| 允许访问的额外目录 | `AppConfig` | `todo.ini` 的 `allowed_dirs` |
| 合并皮肤的界面状态 | 皮肤 Lua 脚本 | `ui_state.inc`：`View`, `CurrentChat`, `Skill`, `ChatOpen`, `Style`, `TasksPromptX/Y`, `ChatPromptX/Y` |

# Interface

## Interfaces

### 内容文件

```markdown
- [ ] 跑对比实验 B <!-- id=d20261020-1 created=2026-10-20 rolled=0 place=lab est=120 -->
```

`place` ∈ {`lab`, `class`, `remote`, `any`}，省略时为 `any`；`est` 为分钟数，可省略。

skill 文件：

```markdown
# Planner
<!-- tools=list_tasks,add_task,read_file -->

系统提示正文……
```

计划文件（`plans/2026-10-20.md`）：

```markdown
# Plan 2026-10-20 <!-- created=2026-10-20T08:10 status=proposed -->

## Blocks
- 09:00-10:00 commute transit remote -> campus
- 10:00-11:40 task lab 跑对比实验 B (d20261020-1)

## Changes
- [1] set_place d20261020-3 remote
- [2] add_task day lab 30 整理组会记录

## Rationale
……
```

时间块行：`- <start>-<end> <kind> <place> <title>[ (<task_id>)]`；日计划 `start` 为 `HH:MM`，周计划为 `Mon HH:MM`；`kind` ∈ {`task`, `fixed`, `commute`, `break`}，`place` ∈ {`lab`, `class`, `remote`, `any`, `transit`}。改动行是规范形式：`set_place <id> <place>`、`set_estimate <id> <分钟>`、`add_task <level> <place> <分钟|-> <title>`。

计划状态：`proposed` → `applied`（全部或部分改动已应用，头部 `applied=1,3` 记录已应用的编号）或 `discarded`。只有 `proposed` 的计划可以 apply 或 discard。

### C++ 接口

```cpp
// src/domain/task.h  (new fields)
enum class TaskPlace { Any, Lab, Class, Remote };
struct Task { /* existing fields */ TaskPlace place = TaskPlace::Any; std::optional<int> estimate_minutes; };

// src/ai/deepseek_client.h
struct ToolCall { std::string id; std::string name; std::string arguments_json; };
struct ChatMessage {
    std::string role;                  // system | user | assistant | tool
    std::string content;
    std::vector<ToolCall> tool_calls;  // assistant only
    std::string tool_call_id;          // tool only
};
struct ToolSpec { std::string name; std::string description; std::string parameters_json; };
struct CompletionOptions { std::vector<ToolSpec> tools; bool json_output = false; };
struct ChatCompletion { std::string content; std::vector<ToolCall> tool_calls; int tokens_in; int tokens_out; long long latency_ms; };
class DeepSeekClient { public: ChatCompletion complete(const std::vector<ChatMessage>&, const CompletionOptions&) const; };

// src/tools/path_policy.h
enum class AccessKind { Read, Write };
struct AccessDecision { std::filesystem::path resolved; bool allowed; bool needs_confirmation; std::string reason; };
class PathPolicy {
public:
    PathPolicy(std::filesystem::path data_dir, std::vector<std::filesystem::path> allowed_dirs);
    AccessDecision check(const std::string& requested, AccessKind kind) const;  // relative paths resolve against data_dir
};

// src/tools/tool.h
struct ToolOutcome { std::string result_text; bool changed_content_repo; std::string summary; };
class Tool {  // abstract
public:
    virtual ~Tool() = default;
    virtual ToolSpec spec() const = 0;
    virtual ToolOutcome execute(const nlohmann::json& arguments) const = 0;  // errors become result_text, not exceptions
};

// src/tools/tool_registry.h
class ToolRegistry {
public:
    void add(std::unique_ptr<Tool> tool);
    std::vector<ToolSpec> specs(const std::vector<std::string>& names) const;
    ToolOutcome execute(const ToolCall& call, const std::vector<std::string>& allowed_names) const;
};
// Tools: list_tasks, add_task, set_task_status, add_task_note, set_task_place (task_tools.*);
//        list_dir, read_file, write_file, replace_in_file (file_tools.*)

// src/prompt/confirm_window.h
struct ConfirmRequest { std::wstring title; std::wstring detail; std::wstring anchor_title; };
class ConfirmWindow { public: bool run(const ConfirmRequest& request) const; };  // Enter: true, Esc/close: false

// src/skills/skill_store.h
struct Skill { std::string name; std::string title; std::vector<std::string> tools; std::string system_prompt; };
class SkillStore {
public:
    explicit SkillStore(std::filesystem::path data_dir);
    void ensure_defaults() const;  // writes assistant.md, planner.md, research.md only if skills/ has no .md file
    std::vector<Skill> list() const;
    Skill load(const std::string& name) const;
};

// src/chat/chat_session.h  (extended)
struct ToolRecord { std::string name; std::string arguments_json; std::string summary; };
struct ChatTurn { /* existing */ std::vector<ToolRecord> tools; };  // tools used before this assistant reply
struct ChatSession { /* existing */ std::string skill; };            // empty: no tools

// src/chat/chat_service.h
class ChatService {
public:
    ChatService(const ChatStore&, const DeepSeekClient&, const TaskRepository&, const SkillStore&, const ToolRegistry&);
    ChatExchange send(ChatSession& session, const std::string& message, bool with_tasks, const Date& today) const;
    // At most kMaxToolRounds = 8 completion calls per message; the last one is sent without tools.
};

// src/planning/plan.h, plan_store.h, planner.h
struct PlanBlock { std::string start; std::string end; std::string kind; std::string place; std::string title; std::optional<std::string> task_id; };
struct PlanChange { int number; std::string op; nlohmann::json arguments; std::string description; };  // op: add_task | set_place | set_estimate
struct Plan { std::string period; std::string created; std::string status; std::vector<PlanBlock> blocks; std::vector<PlanChange> changes; std::vector<int> applied; std::string rationale; };
class PlanStore { public: void save(const Plan&) const; std::optional<Plan> load(const std::string& period) const; };
struct PlanProposal { Plan plan; ChatCompletion completion; size_t prompt_chars; };
class Planner {   // needs the API key
public:
    PlanProposal propose(TaskLevel scope, const Date& date) const;  // DeepSeek json_object; throws TodoError on invalid JSON or schema mismatch
};
class PlanApplier {   // no API key; applies through AddTaskTool / SetTaskPlaceTool
public:
    std::vector<std::string> apply(Plan& plan, const std::vector<int>& numbers, const Date& today) const;  // returns applied descriptions
};
// PlanParser (JSON → Plan), PlanBlockCodec, PlanChangeCodec, PlanPeriod, PlanProfile (writes example profile files when missing)
```

## Inputs

```text
todo.exe --config <todo.ini> <command>

  place <task_id> <any|lab|class|remote>
  estimate <task_id> <minutes|none>
  skill list
  ask [--with-tasks] [--skill NAME] <question>
  chat new [--skill NAME] <title>
  chat send <chat_id> [--with-tasks] <message>     会话沿用创建时的 skill
  plan day|week [--date YYYY-MM-DD]                生成并保存计划
  plan show day|week [--date YYYY-MM-DD]
  plan apply <period> <all|n,n,...>
  plan discard <period>
```

`todo.ini` 新增 `allowed_dirs = <dir>; <dir>`（可为空）。

## Outputs

新增记录：

```text
TASK	... 原有字段 ...	<place>	<estimate_minutes|>
SKILL	<name>	<title>	<tools comma-separated>
TOOL	<name>	<summary>	<allowed|denied|error>
PLAN	<period>	<status>	<created>
BLOCK	<start>	<end>	<kind>	<place>	<title>	<task_id|>
CHANGE	<number>	<description>	<applied|>
RATIONALE	<text>
```

## State Changes

- 文件工具写入：
  - 内容仓库内：直接写入，命令结束时一并提交。
  - 允许目录内：先由 `ConfirmWindow` 确认，拒绝时工具返回 "denied by user"。
  - 其它路径：返回 "outside allowed directories"。
- 计划：`plan` 写入 `status=proposed`；`plan apply` 应用选中的改动，并把状态改为 `applied`；`plan discard` 改为 `discarded`。

# Flow

## Main Flow

带 skill 的对话：

```text
chat send / ask
  → ChatService::send
      skill = SkillStore::load(session.skill)；system = skill.system_prompt + 日期 (+ 任务)
      循环最多 8 轮：
        DeepSeekClient::complete(messages, {tools = registry.specs(skill.tools)})
        → 无 tool_calls：得到最终回答，结束
        → 每个 ToolCall：ToolRegistry::execute → PathPolicy / ConfirmWindow → ToolOutcome
          追加 assistant(tool_calls) 与 tool(result) 消息；记录 ToolRecord
      ChatStore::save；SkinOutput: TOOL…, CHAT, TURN…, USAGE
  → GitClient::commit_all（会话文件与工具对内容仓库的修改）
```

规划：

```text
plan day
  → Planner::propose
      输入：今日 / 本周 / 长期任务（含 place、est）、profile/places.md、profile/schedule.md、planner skill 的系统提示、JSON 示例
      DeepSeekClient::complete(json_output = true) → 校验字段 → Plan
  → PlanStore::save → 提交 → PLAN / BLOCK / CHANGE / RATIONALE
皮肤 Plan 标签页：显示时间块与改动；点击改动切换是否选中；Apply → plan apply <period> <选中的编号>
```

合并皮肤：

```text
Main.ini
  ├─ 头部：日期、星期、今日完成进度
  ├─ 标签页 Today / Week / Long / Plan（task_view.lua、plan_view.lua）
  ├─ 提问栏：Ask、skill 选择、Tasks 开关、会话列表（chat_view.lua）
  └─ 对话区：有回答时展开，可折叠；显示 TOOL 记录
```

# Code Mapping

## Modules

| 模块 | 文件 |
|---|---|
| Task Domain / Storage | `src/domain/task.h`, `src/domain/task_place.h/.cpp`, `src/storage/task_file.cpp` |
| AI Transport | `src/ai/deepseek_client.h/.cpp` |
| Access Policy | `src/tools/path_policy.h/.cpp` |
| Tools | `src/tools/tool.h`, `src/tools/tool_registry.h/.cpp`, `src/tools/task_tools.h/.cpp`, `src/tools/file_tools.h/.cpp` |
| Confirm | `src/prompt/confirm_window.h/.cpp` |
| Skills | `src/skills/skill_store.h/.cpp`, `src/skills/default_skills.h/.cpp` |
| Chat | `src/chat/chat_session.h`, `src/chat/chat_store.cpp`, `src/chat/chat_service.h/.cpp` |
| Planning | `src/planning/plan.h`, `src/planning/plan_period.h/.cpp`, `src/planning/plan_block_codec.h/.cpp`, `src/planning/plan_change_codec.h/.cpp`, `src/planning/plan_parser.h/.cpp`, `src/planning/plan_store.h/.cpp`, `src/planning/plan_profile.h/.cpp`, `src/planning/planner.h/.cpp`, `src/planning/plan_applier.h/.cpp`；验证用 `probes/plan_probe.cpp` |
| CLI | `src/cli/command.h`, `src/cli/command_parser.cpp`, `src/cli/command_runner.h/.cpp`, `src/cli/skin_output.h/.cpp`, `src/main.cpp` |
| Config | `src/config/app_config.h/.cpp`, `todo.ini` |
| Desktop Skin | `rainmeter/ToDo/Main/Main.ini`, `rainmeter/ToDo/@Resources/Scripts/{task_view,chat_view,plan_view}.lua`, `rainmeter/ToDo/@Resources/style.inc`, `rainmeter/ToDo/@Resources/ui_state.inc`；删除 `rainmeter/ToDo/Tasks/`、`rainmeter/ToDo/Chat/` |
| Install | `scripts/install_skin.ps1` |
| Design | `design/mockups/*.html`, `design/mockups/*.png` |
| Validation | `scripts/smoke_test.py` |

## Interfaces

| 接口 | 文件 |
|---|---|
| 任务元数据 `place` / `est` | `src/storage/task_file.cpp` |
| tools 请求与 `tool_calls` 响应 | `src/ai/deepseek_client.cpp` |
| 路径判定 | `src/tools/path_policy.cpp` |
| skill 文件格式 | `src/skills/skill_store.cpp` |
| 会话文件 `skill=` 与 `## tool` | `src/chat/chat_store.cpp` |
| 计划文件与 JSON 结构 | `src/planning/plan_store.cpp`, `src/planning/planner.cpp` |
| 新命令与记录 | `src/cli/command_parser.cpp`, `src/cli/skin_output.cpp` |

## State

| 状态 | 所有者文件 |
|---|---|
| 任务地点、时长 | `src/storage/task_repository.cpp` |
| skill 文件 | `src/skills/skill_store.cpp` |
| 会话与工具记录 | `src/chat/chat_store.cpp` |
| 计划 | `src/planning/plan_store.cpp` |
| 允许目录 | `src/config/app_config.cpp` |
| 皮肤界面状态 | `rainmeter/ToDo/@Resources/ui_state.inc` |

# Implementation

## Phase 1

Phase Status: completed

Round: 1

### Goal

做出合并皮肤的 2～3 版外观效果图，由使用者选定一版。

### Files

`design/mockups/variant-a.html`, `design/mockups/variant-b.html`, `design/mockups/variant-c.html` 及对应截图

### Structures

每版都包含：任务行（状态图标、地点标签、时长、顺延次数）、头部进度、Plan 标签页、提问栏、展开后的对话区（含工具调用行）

### Affected Modules

Design

### Dependencies

无

### Observation

- 每版的截图按 420 像素宽、实际字体渲染。使用者选定后，记录所选版本的配色、字号和间距数值。

### Result

选定 `variant-c`（紧凑半透明）为默认。三版配色均实现为 `style-a.inc` / `style-b.inc` / `style-c.inc`，由 `Style` 切换。`style-c.inc` 取以下数值：

| 项 | 值 |
|---|---|
| 背景 | `10,12,16,158`，描边 `255,255,255,26`，圆角 10，宽 420，内边距 10 / 12 |
| 文字 / 次要文字 / 强调 | `242,242,242` / `154,160,168` / `143,211,255` |
| 状态色 done / blocked | `142,224,142` / `255,138,122` |
| 地点色 lab / class / remote | `94,234,212` / `196,181,253` / `253,186,116`；16 px 圆点内写 L / C / R |
| 状态符号 | todo `○`，doing `◐`，done `●`，blocked `⊘` |
| 字号 | 正文 12.5，标题 15 粗体，元数据与工具行 10.5，小标签 10 |
| 行高 | 任务行 25，时间轴行 24，改动行 22 |
| 进度条 | 每个任务一段，高 3，间隔 3；done 绿，doing 强调色，其余 `255,255,255,26` |
| 标签页 | 右上角分段控件，选中项底色 `143,211,255,46` |
| 字体 | Segoe UI / Microsoft YaHei UI；工具行 Consolas |

## Phase 2

Phase Status: completed

Round: 1

### Goal

任务地点与时长：数据格式、`place` / `estimate` 命令、`TASK` 记录新字段。

### Files

`src/domain/task.h`, `src/domain/task_place.h/.cpp`, `src/storage/task_file.cpp`, `src/cli/command_parser.cpp`, `src/cli/command_runner.h/.cpp`, `src/cli/skin_output.cpp`, `src/chat/chat_service.cpp`, `CMakeLists.txt`, `scripts/smoke_test.py`

### Structures

`TaskPlace`, `TaskPlaceCodec`

### Affected Modules

Task Domain / Storage, CLI

### Dependencies

无

### Observation

- smoke 场景：设置地点和时长后打印任务文件与 `TASK` 记录；已有的不含新字段的文件可以原样解析，往返校验和不变。

## Phase 3

Phase Status: completed

Round: 2

### Goal

DeepSeek 传输层支持 tools、`tool_calls` 和 JSON 输出。

### Files

`src/ai/deepseek_client.h/.cpp`, `src/chat/chat_service.cpp`, `probes/deepseek_probe.cpp`（验证用独立可执行文件 `deepseek_probe`）, `CMakeLists.txt`

### Structures

`ToolCall`, `ToolSpec`, `CompletionOptions`, 扩展后的 `ChatMessage` 与 `ChatCompletion`

### Affected Modules

AI Transport

### Dependencies

Phase 2（共享 `src/chat/chat_service.cpp`）

### Observation

- 真实调用：给出一个示例工具，打印返回的 `tool_calls`；回传 tool 结果后打印最终回答；`json_output` 调用的结果可以被解析。打印延迟和 token。

## Phase 4

Phase Status: completed

Round: 3

### Goal

路径判定、确认窗口、任务工具和文件工具、工具注册表、`allowed_dirs` 配置。

### Files

`src/tools/*`, `src/prompt/confirm_window.h/.cpp`, `src/config/app_config.h/.cpp`, `todo.ini`, `CMakeLists.txt`, `scripts/smoke_test.py`

### Structures

`PathPolicy`, `AccessDecision`, `Tool`, `ToolRegistry`, `ToolOutcome`, `ConfirmWindow`

### Affected Modules

Access Policy, Tools, Confirm, Config

### Dependencies

Phase 3

### Observation

- 打印路径判定结果表，覆盖：内容仓库内相对路径、`..` 越界、允许目录内、允许目录外、符号链接或大小写不同的路径。
- 确认窗口：用 `send_text.ps1` 分别按 Enter、Esc，打印文件写入前后的校验和。

## Phase 5

Phase Status: completed

Round: 4

### Goal

skill 文件与默认 skill；带工具调用的对话循环；会话文件记录 skill 与工具调用；`skill list`，以及 `ask` / `chat new` 的 `--skill` 参数。

### Files

`src/skills/*`, `src/chat/chat_session.h`, `src/chat/chat_store.cpp`, `src/chat/chat_service.h/.cpp`, `src/cli/command_parser.cpp`, `src/cli/command_runner.h/.cpp`, `src/cli/skin_output.h/.cpp`, `src/main.cpp`, `CMakeLists.txt`, `scripts/smoke_test.py`

### Structures

`Skill`, `SkillStore`, `ToolRecord`

### Affected Modules

Skills, Chat, CLI

### Dependencies

Phase 4

### Observation

- 真实调用 assistant skill：要求它读取一个 research 笔记并添加一条任务。打印 `TOOL` 记录、会话文件、任务文件和 `git log`；打印每条消息的轮数和总延迟。
- 无 skill 的会话不发送 tools 字段：打印请求体中是否包含 `tools`。

## Phase 6

Phase Status: completed

Round: 5

### Goal

规划：`plan day|week`、`plan show`、`plan apply`、`plan discard`；示例 `profile/places.md` 与 `profile/schedule.md`。

### Files

`src/planning/*`, `src/skills/default_skills.cpp`, `src/cli/command_parser.cpp`, `src/cli/command_runner.h/.cpp`, `src/cli/skin_output.h/.cpp`, `CMakeLists.txt`, `scripts/smoke_test.py`

### Structures

`Plan`, `PlanBlock`, `PlanChange`, `PlanStore`, `Planner`

### Affected Modules

Planning, CLI

### Dependencies

Phase 2, Phase 5

### Observation

- 用一份含 lab / class / remote 任务的示例数据真实生成日计划。打印时间块，并按块统计：remote 与校园之间的切换次数，以及通勤块总时长。
- `plan apply 1,3` 后打印任务文件与计划文件状态；对模型返回的非法 JSON 输出 `ERR`，且不写入任何文件。

### Result

`scripts/smoke_test.py plan`（2026-10-20，周二，6 个日任务、2 个周任务、1 个长期任务）：

| 运行 | 计划 | 时间块 | remote↔校园切换 | 通勤块 / 分钟 | 延迟 ms | tokens in / out |
|---|---|---|---|---|---|---|
| 1 | day | 12 | 2 | 2 / 75 | 3011 | 1127 / 634 |
| 2 | day | 11 | 1 | 1 / 60 | 2863 | 1127 / 565 |
| 2 | week（周二至周五） | 24 | 3 | 6 / 320 | 3991 | 1161 / 987 |

- `plan apply 2026-10-20 1,3`：只有编号 1、3 的估时写入任务文件，计划头部变为 `status=applied applied=1,3`，`CHANGE` 记录第三字段为 `applied`；再次 apply 或 discard 返回 `ERR`。
- `plan_probe` 对非 JSON、缺少 `blocks`、非法 kind、日计划中的周时间、非法 task_id、非法 op 均返回 `ERR`；数据目录校验和前后相同。
- 模型给出的通勤块时长不总是 60 分钟（运行 1 有 15 分钟的通勤块），见 `issues.md` ISSUE-001。

## Phase 7

Phase Status: completed

Round: 6

### Goal

按 `variant-c` 的布局实现合并皮肤 `ToDo\Main`：任务、Plan、提问栏、可折叠对话区；点击头部样式标记在 A / B / C 三套配色间循环；安装脚本改为停用 Tasks / Chat、启用 Main。

### Files

`rainmeter/ToDo/Main/Main.ini`, `rainmeter/ToDo/@Resources/Scripts/task_view.lua`, `rainmeter/ToDo/@Resources/Scripts/chat_view.lua`, `rainmeter/ToDo/@Resources/Scripts/plan_view.lua`, `rainmeter/ToDo/@Resources/style-{a,b,c}.inc`, `rainmeter/ToDo/@Resources/ui_state.inc`, `scripts/install_skin.ps1`；删除 `rainmeter/ToDo/Tasks/`、`rainmeter/ToDo/Chat/`

### Structures

`Main.ini` 的分区与分组；`plan_view.lua`

### Affected Modules

Desktop Skin, Install

### Dependencies

Phase 1, Phase 2, Phase 5, Phase 6

### Observation

- 截图：任务视图、Plan 视图、对话折叠与展开、带 `TOOL` 行的回答，与选定的效果图并排对比。
- 用 bang 走一遍添加任务、设置地点、提问、生成计划并应用改动，打印内容仓库 `git log`。
- 由使用者在桌面上实际操作确认。

### Result

- `rainmeter/ToDo/Main/Main.ini`（ASCII）与 `task_view.lua`、`plan_view.lua`、`chat_view.lua`；状态与地点标记用 Shape 绘制。`style.inc` 由 `style-a.inc`、`style-b.inc`、`style-c.inc` 取代，`Main.ini` 按 `ui_state.inc` 的 `Style` 包含其一。`rainmeter/ToDo/Tasks/`、`rainmeter/ToDo/Chat/` 已删除。
- `install_skin.ps1` 停用 `ToDo\Tasks`、`ToDo\Chat`，激活 `ToDo\Main`，并给已有的 `ui_state.inc` 补上 `Skill`、`ChatOpen`、`Style`。
- 沙盒内容仓库上的截图观测：任务视图、Plan 日 / 周视图（分页、rationale、取消勾选后 `apply 1`）、对话展开与折叠（窗口高度 723 → 340）、带 `TOOL` 行的回答、会话列表、样式 A / B / C 循环。用 bang 加 `send_text.ps1` 走通添加任务、设置地点、提问（`set_task_place` 工具）、生成计划、应用改动；沙盒内容仓库每步都有对应提交。
- 之后用真实 `todo.ini` 重新安装，皮肤移到原 `ToDo\Tasks` 的位置（2301, 90）。皮肤启动时运行 `skill list`，在 `ToDo-data` 写入默认 skill（提交 `skill defaults`）。真实内容仓库中还没有计划与 profile 文件。
- 使用者已在桌面上操作（生成并查看日计划、提问）。

## Phase 8

Phase Status: completed

Round: 7

### Goal

修正桌面使用中发现的规划问题：
- 规划请求与对话系统提示写入当前时间；今天的日计划只排当前时间之后；标题或备注写明某一天的任务只排在那一天。
- `plan day|week --chat <chat_id>`：把该会话最近的轮次放进规划请求；皮肤点 plan / re-plan 时传入当前会话。
- 对话工具 `propose_plan`：模型直接给出时间块、改动和理由，经 `PlanParser` 校验后保存为 `proposed` 计划；皮肤随后切到 Plan 标签显示。默认 skill `planner` 与 `assistant` 可用。
- `CHANGE` 记录增加第 5 字段：改动所指任务的标题（`add_task` 为空）；Plan 标签用它显示改动。

### Files

`src/planning/planner.h/.cpp`, `src/tools/plan_tools.h/.cpp`, `src/chat/chat_service.cpp`, `src/skills/default_skills.cpp`, `src/cli/command.h`, `src/cli/command_parser.cpp`, `src/cli/command_runner.h/.cpp`, `src/cli/skin_output.h/.cpp`, `CMakeLists.txt`, `rainmeter/ToDo/@Resources/Scripts/{plan_view,chat_view}.lua`, `scripts/smoke_test.py`

### Structures

`PlanRequestContext`（当前时间、会话轮次）；`ProposePlanTool`

### Affected Modules

Planning, Tools, Chat, Skills, CLI, Desktop Skin

### Dependencies

Phase 6, Phase 7

### Observation

- 真实调用：在会话中说明出门时间后 `plan day --chat <id>`，打印时间块起点与当前时间。
- 带 `planner` skill 的会话要求给出计划：打印 `TOOL propose_plan` 记录、计划文件状态与 `plan show` 输出。
- `CHANGE` 记录的标题字段；皮肤截图。

### Result

- 真实数据的副本上，在会话中说明出门时间后运行 `plan day --chat <id>`：首块从当前时间开始，出门后先排通勤块，固定课程按 profile 排入，标明周日的任务未排入当天；2122 / 509 tokens，2825 ms。
- `ask --skill planner` 要求重排：工具依次为 `read_file` ×2、`list_tasks`、`propose_plan`（11 块、5 改动），`plan show day` 显示新计划，`CHANGE` 第 5 字段为任务标题；6835 / 997 tokens，5964 ms，3 轮。对话系统提示加入星期之前，模型曾在周二排入周一 / 周四的课。
- 皮肤：沙盒中由对话产生计划后自动切到 Plan 标签；真实数据上改动行显示任务名；刷新后对话滚动到最新问题。
- `scripts/smoke_test.py plan` 复跑：日计划 1 次切换、通勤 60 分钟；6 类非法回复仍输出 `ERR`，校验和不变。
- 真实 `ToDo-data/skills/planner.md`、`assistant.md` 已加入 `propose_plan`。

## Phase 9

Phase Status: completed

Round: 8

### Goal

以 Plan 为主页：顶部只保留 Plan | Tasks 两个标签（默认 Plan），Tasks 内用 day / week / long 切换；Plan 中关联任务的时间块显示任务状态并可点击推进；Plan 底部列出计划未包含的未完成日 / 周任务。

### Files

`src/cli/command_runner.h/.cpp`, `rainmeter/ToDo/Main/Main.ini`, `rainmeter/ToDo/@Resources/Scripts/{task_view,plan_view}.lua`, `rainmeter/ToDo/@Resources/ui_state.inc`

### Structures

`plan show|day|week|apply|discard` 在计划记录之后输出该周期锚定日期的日、周任务 `TASK` 记录（没有计划时也输出）；`ui_state.inc` 的 `View` 取 `plan | tasks`，新增 `TaskScope`（day / week / long），旧值 `View=day|week|long` 按 `tasks` 加对应 scope 读取。

### Affected Modules

CLI, Desktop Skin

### Dependencies

Phase 8

### Observation

- 真实数据：Plan 视图显示任务块的状态标记与 NOT IN PLAN 列表；Tasks 视图的 week 列表。
- 沙盒副本：点击时间块两次（todo → doing → done）与未排入任务一次，内容仓库出现对应的 `status` 提交，截图中显示半圆、删除线与新状态。

### Result

- 以上观测均符合预期。验证过程中一次沙盒安装失败后点击命令落到了真实数据，把 `d20261020-1` 从 blocked 改为 todo（提交 c2ab46e），已用 `status d20261020-1 blocked` 还原（f5d3060）。

## Phase 10

Phase Status: completed

Round: 9

### Goal

让对话能可靠地修改周计划：周计划时间块的结束时间可以带与开始相同的星期前缀；`propose_plan` 的说明同时写明日、周两种时间格式；新增默认 skill `replan`，先读现有计划文件，只改用户要求的部分，再调用一次 `propose_plan`。

### Files

`src/planning/plan_block_codec.cpp`, `src/planning/plan_format.h/.cpp`, `src/tools/plan_tools.cpp`, `src/skills/default_skills.cpp`, `src/skills/skill_store.h`

### Affected Modules

Planning, Tools, Skills

### Dependencies

Phase 9

### Observation

- `plan_probe`：周计划 end 为 `Tue 12:00`（同日）被接受并规范为 `12:00`；`Wed 12:00`（跨日）与日计划中的 `Tue 12:00` 输出 `ERR`。
- 沙盒副本：`ask --skill planner` 生成周计划，`propose_plan` 一次成功（28 块，4 轮，10812 / 1829 tokens，9418 ms）；修复前同类请求 9 次工具调用均因时间格式失败。
- 沙盒副本：`ask --skill replan` 要求只改周三下午：读取周计划、今日计划、两份 profile 后一次 `propose_plan`；前后对比 28 块中 24 块逐字不变，周三下午 4 块换为 3 块（3 轮，11826 / 1796 tokens，7592 ms）。
- 空 skills 目录写入的默认 `replan.md` 与 `ToDo-data/skills/replan.md` 相同。

### Result

以上观测均符合预期。

## Phase 11

Phase Status: completed

Round: 10

### Goal

桌面吉祥物（女仆鲸鱼娘）分层动画：独立皮肤 `ToDo\Mascot`。待机时全身轻微摇晃与呼吸，长发、尾巴、呆毛各自绕根部摆动；随机眨眼；点击头部出现笑眼和腮红（摸头）；点击身体或收到 AI 回答时端碗吃饭（张嘴、咀嚼）；Plan 标签页生成计划期间低头在本子上写字。透明背景，只切换或变换单个图层，不做整图换帧与交叉淡化。

### Files

`design/mascot/source/*`, `design/mascot/split_layers.py`, `design/mascot/export_skin_assets.py`, `rainmeter/ToDo/@Resources/Mascot/*.png`, `rainmeter/ToDo/@Resources/Mascot/layout.inc`, `rainmeter/ToDo/Mascot/Mascot.ini`, `rainmeter/ToDo/@Resources/Scripts/mascot_view.lua`, `rainmeter/ToDo/@Resources/Scripts/{plan_view,chat_view}.lua`, `scripts/install_skin.ps1`

### Structures

- 图层（自下而上）：hair_back、tail、ahoge、body | body_eat | body_write、eyes_half | eyes_closed | eyes_down、mouth_open | mouth_chew、face_happy。静态图层同一画布、同一位置。
- 长发、尾巴、呆毛在导出时按角度预先旋转成帧（长发 0.5°×9、尾巴 1°×17、呆毛 2°×19），各帧裁到不透明区域；运行时只显示一帧。整体摇晃用 `!MoveMeter` 平移（±1.5 px，写字时上下 ±1 px，摸头时跳起 4 px）。
- `layout.inc`：画布尺寸、边距、各部件帧数与角度步长、点击区域；`frames_<part>.inc`：每帧一个 Image meter。均由导出脚本生成。
- `mascot_view.lua` 的公开函数：`Pat()`、`Eat()`、`StartWriting()`、`StopWriting()`；Main 通过 `!CommandMeasure MeasureMascot "<函数>" "ToDo\Mascot"` 调用。动画状态只在该脚本内。

### Affected Modules

Design, Desktop Skin, Install

### Dependencies

Phase 9

### Observation

- 拆分脚本输出：各图层叠回后与原图的平均差异、未覆盖像素数。
- 截图：待机、眨眼闭眼帧、摸头、吃饭张嘴与咀嚼、写字；夸大摆动角度时无空洞。
- 由 Main 触发：re-plan 时进入写字、结束后恢复；对话回答后吃饭。

### Result

- 拆分：各补丁层叠回后区域内平均差异 0.4–1.4（0–255）；hair_back + tail + ahoge + body 叠回主图未覆盖像素 0，全图平均差异 4.09；所有生成图与主图在 (0, 0) 对齐。
- CPU（Rainmeter 进程，10 s 平均，单核百分比）：

| 方案 | CPU |
|---|---|
| 无吉祥物 | 6.6% |
| 每帧对 6 个图层设置 TransformationMatrix，25 fps | 74–99% |
| 同上，10 fps | 26.6% |
| 不设置变换（只显示 / 隐藏） | 8.0% |
| 变量 + DynamicVariables 驱动变换 | 88.3% |
| 只变换尾巴 + 呆毛 / 只变换长发 / 只变换身体组 | 43.4% / 24.6% / 12.3% |
| 每帧 `!SetOption X`（无变换） | 10.9% |
| 每帧 `!MoveMeter` | 4.1% |
| 采用方案：预旋转帧 + `!MoveMeter`，25 fps | 4.2% |

  开销来自带非单位变换矩阵的图片绘制，而不是选项读取；因此改为导出时预旋转，去掉了整体呼吸缩放。
- 截图：待机两帧（含眨眼半闭）、摸头、张嘴、咀嚼、写字均正常；Main 的 `OnRunFinished` 调用 `StopWriting()` 后恢复待机，Rainmeter 日志无错误。
- 素材：`rainmeter/ToDo/@Resources/Mascot` 共 58 个文件，约 1.2 MB。

## Phase 12

Phase Status: completed

Round: 11

### Goal

吉祥物：帧率从 25 提到 50 fps，摆动帧更细（长发 0.25°×17、尾巴 0.5°×31、呆毛 1°×37）；双击开关 ToDo 窗口；右键菜单加"显示 / 隐藏 ToDo"与"置顶 开 / 关"（`!ZPos 2 / 0`，Rainmeter 会记住）；吃饭咀嚼时眯眼（从 03_happy_blush 截出的 `eyes_happy`）。

### Files

`design/mascot/{split_layers,export_skin_assets}.py`, `rainmeter/ToDo/Mascot/Mascot.ini`, `rainmeter/ToDo/@Resources/Scripts/mascot_view.lua`, `rainmeter/ToDo/@Resources/Mascot/*`

### Observation

- CPU（Rainmeter 进程，10 s 平均）：50 fps 时 4.2% 单核，与 25 fps 相同。
- 点击身体 1.3 s 后截图为眯眼咀嚼，动画时间与实际时间一致（`Update=#MascotFrameMs#` 生效）。
- 素材 99 个文件，约 2.0 MB。

### Result

以上观测均符合预期。

## Phase 12b

Phase Status: completed

Round: 11

### Goal

吉祥物缩小到 160 px 宽；去掉整数像素的整体平移（表现为待机抖动）：下半身固定，上半身绕腰（原图 y=960，手的下方）倾斜，帧在导出时预先旋转（倾角 -1°..+2.5°，0.25° 一帧）；长发与呆毛按"倾角 × 自身摆角"成帧，尾巴挂在下半身；咀嚼时上半身多倾 2°；修正吃饭时出现两张嘴（`eyes_happy` 只取眼睛和两颊，避开嘴、碗沿和勺子）。

### Observation

- 待机连续 12 张截图（间隔 300 ms）：下半身区域相邻差异均值 0.06（噪声水平），头部区域 8.28。
- 吃饭叠层：碗沿、勺子区域与不加眼睛补丁时的差异 0.03 / 0.01；嘴只剩咀嚼的一张。
- 514 帧，约 7.4 MB；窗口 180×216；Rainmeter 进程 CPU 5.5% 单核，工作集 122 MB。

### Result

以上观测均符合预期。

## Phase 13

Phase Status: completed

Round: 12

### Goal

1. 头部单独绕颈部转动：头部角度 = 上半身倾角 + 歪头（吃饭时 5°、摸头时 2.5°）；脸部补丁、眼睛、长发、呆毛随头部转动，吃饭 / 写字的手臂与道具画在头部之前。
2. 眼睛跟随鼠标：自建 Rainmeter 插件 `CursorPos.dll`（`plugins/cursor_pos/cursor_pos.cpp`，导出 Initialize / Reload / Update / GetString / Finalize，内部调用 `GetCursorPos`，不依赖 SDK），由安装脚本复制到 `%APPDATA%\Rainmeter\Plugins`。瞳孔放在以眼白为容器的 Image meter 中，按光标方向最多移动 ±2 / ±1 皮肤像素。

### Files

`plugins/cursor_pos/cursor_pos.cpp`, `CMakeLists.txt`, `scripts/install_skin.ps1`, `design/mascot/{register_layer,split_layers,export_skin_assets}.py`, `design/mascot/source/17_head*.png`, `18_torso_no_head.png`, `19_irises.png`, `20/21_eyes_white_*_unaligned.png`, `rainmeter/ToDo/Mascot/Mascot.ini`, `rainmeter/ToDo/@Resources/Scripts/mascot_view.lua`, `rainmeter/ToDo/@Resources/Mascot/*`

### Observation

- 配准：生成的头部图需缩放 0.9625、上移 18 px；面部区域平均差异 35.36 → 18.6，叠加后无重影。去头身体图与主图偏差 ≤ 1 px；瞳孔图对齐。两张眼白图比主图眼睛低 60–80 px 且形状不同，未使用；眼白改由主图生成（瞳孔内部填白，保留外圈与压在瞳孔上的上睫毛）。
- 容器内 meter 的 `GetX()` 返回值叠加了两次容器偏移；瞳孔位置改为读取导出的 `RestX` / `RestY` 并用 `!SetOption` 设置。
- 截图：光标在左上 / 右下时瞳孔分别偏向左上 / 右下；吃饭时头部倾斜，碗勺在头前；写字、摸头正常。
- 825 个图层帧，约 9.5 MB；窗口 182×212；Rainmeter 进程 CPU 6.1% 单核，工作集 133 MB。

### Result

以上观测均符合预期。

## Phase 14

Phase Status: completed

Round: 13

### Goal

吉祥物动作更自然、更流畅：角度换成更细的预旋转帧（头 0.2°、上身 0.1°、长发 0.15°、尾巴 0.3°），长发改为绕颈部摆动（一维帧），呆毛一维帧加随头部的平移；所有角度改为阻尼弹簧追随目标，长发与呆毛受头部角加速度推动（滞后与回弹），待机目标叠加不等周期波与随机事件（重心移动、歪头、甩尾）；长发摆幅约 ±4°。帧时间由插件提供的高精度时钟测得，Update=15（Windows 计时器 15.6 ms 一跳）。眼白改为整个眼眶（瞳孔与眼角白色的凸包，加下眼睑细线），瞳孔连同深色边框在眼眶内移动，范围 ±4 / ±2 皮肤像素。

### Observation

- 实时 TransformationMatrix（50 fps）对照：只帧切换 5.9%；尾巴 + 呆毛 13.9%；长发 19.7%；头 11.6%；全部约 9 层 26.3%（单核）。因此保留预旋转帧。
- 连续截图（约 140 次 / 秒，5 s）相邻不同的比例：调整前 13%（约 18 次可见变化 / 秒）→ 细帧 20%（31 / 秒，受 20 ms 落在两跳计时器限制）→ Update=15 加实测时钟 41%（58 / 秒）。
- 眼睛：上一版把眼白放在瞳孔轮廓内部，原瞳孔外圈不动，表现为眼白在眼黑里；改为眼眶整体为白、瞳孔整体移动后，光标在左上 / 右下时瞳孔分别偏左 / 偏右，空出的一侧露出眼白。白色检测排除偏暖的浅肤色（否则凸包吞入脸颊）。
- 939 个 meter、893 张帧图，约 8.3 MB；Rainmeter 进程 CPU 7.0% 单核，工作集 122 MB。

### Result

以上观测均符合预期。

## Phase 15

Phase Status: completed

Round: 14

### Goal

鲸鳍（耳朵）可动：去鳍头部（22，已对齐）与左右鳍（24 / 23，由 `register_part.py` 对齐：左鳍上移 43 px；右鳍缩放 0.94、旋转 -6°、上移 44 px）分层，鳍画在头部之后、绕各自根部按 0.5° 预旋转，随头部平移；弹簧驱动：头部转动时滞后、待机轻颤、每 3–8 s 一侧或两侧抖动、摸头时扑扇、吃饭时下垂。同时：瞳孔水平最大移动 4 → 3 px；头部待机自摆约 ±2°，随机歪头 2–4°（头部帧范围 -6°..+8°）。

### Observation

- 截图：待机各帧鳍角度不同，摸头相邻两帧鳍位置变化，吃饭时下垂；鳍画在头部前时根部截面外露，画在头部后时与原图一致。
- 1299 个 meter、1242 张帧图，约 10.5 MB；可见变化约 60 次 / 秒；Rainmeter 进程 CPU 8.4% 单核，工作集 140 MB。

### Result

以上观测均符合预期。

## Phase 16

Phase Status: completed

Round: 15

### Goal

头发与上半身动作更自然：长发由刚性旋转改为弯曲（每行横向平移，顶部 y≤420 不动，至发梢按 ((y-420)/795)^1.4 增大，81 帧，发梢 ±60 源像素），弹簧驱动（随头倾斜、头部转动时发梢惯性滞后、不规则风）。上半身改为绕胸口中点（y=830）倾斜，待机约 ±1.6°、重心移动 ±0.6°，脖子与腰部接缝位移均小于半个皮肤像素；摸头时上半身 ±3° 左右摇晃（约 1.7 Hz，逐渐衰减），头部各层按整像素跟随脖子，耳朵与呆毛使用相同的整像素头部偏移加各自的亚像素偏移。

### Observation

- 头部各层加平移矩阵（每帧变化）：头部组 +11.5%，再加长发 +18.1% 单核；且矩阵一旦设置，清空（空字符串）后待机仍保持约 38%，即无法回到快速绘制路径。因此头部层只用 `!MoveMeter`，矩阵只用于耳朵与呆毛。
- 最终：待机 16.7%、连续摸头 15.2%、摸头后待机 14.1%（单核）；1495 个 meter、约 9.6 MB。
- 截图：摸头连续帧中上半身与头同步左右倾斜；长发弯曲帧在待机中变化。

### Result

以上观测均符合预期。待机 CPU 比 Phase 15 的 12.2% 高约 4 个百分点，主要来自 meter 数量增加（倾斜帧 17 → 71）。


## Phase 17

Phase Status: completed

### Goal

吃饭时头部倾斜更明显。

### Change

- `mascot_view.lua`：`EAT_TILT` 5° → 10°。
- `export_skin_assets.py`：头部帧范围 -6..+8° → -6..+13°（`HEAD = Angles(-6.0, 0.25, 77)`），覆盖吃饭倾斜加身体前倾。

### Result

导出：1735 meters，1658 frames，11062 KiB。吃饭截图中头部倾斜约 10°，勺子与嘴的位置仍然对齐。待机 CPU 16.6%（单核）。

## Phase 18

Phase Status: completed

### Goal

拖动时吉祥物被甩动；ToDo 窗口显示时看向它；无人使用或全屏程序在前台时睡觉并降低 CPU；实测换帧方式（sprite sheet + ImageCrop 等）能否降低 CPU。

### Change

- `plugins/desktop_state/desktop_state.cpp`（新插件 DesktopState）：被监视窗口的可见性与矩形、空闲毫秒数、前台是否全屏；`CMakeLists.txt` 与 `scripts/install_skin.ps1` 一并构建、安装。
- `Mascot.ini`：`MeasureDesktop`，加载时监视 `ToDo\Main`；`Update` 按 `MascotSleep`（`ui_state.inc`）在 15 与 250 之间选择。
- `mascot_view.lua`：窗口加速度推动各弹簧；视线目标在 ToDo 窗口与光标之间选择；入睡、睡眠重载、唤醒重载。
- `ui_state.inc`：新增 `MascotSleep=0`。

### Observation

- 换帧方式实验（`docs/measurements.md`）：显示 / 隐藏与静态相当，ImageName / ImageCrop / 容器滚动高出 3–4 倍；隐藏 meter 数量无可见影响。sprite sheet 方案不采用。
- `!SetOption Rainmeter Update` 运行时不生效；Update=15 下停止 Lua 与换帧只从 17.1% 降到 13.5%；Update=250 为 5.1%（关闭吉祥物 4.0%）。因此睡眠通过重载皮肤切换 Update。
- 截图：入睡后闭眼低头；`Pat()` 唤醒 1.32 s 后新皮肤就绪并眨眼；空闲桌面下 4 s 后再次入睡（`MascotSleep` 0 → 1）。`!Move` 连续左移后头部与长发甩动、随后回摆。ToDo 显示与隐藏时眼珠方向不同。

### Result

以上观测均符合预期。醒着时 CPU 不变（约 17%），睡着时 5.1%。

## Phase 19

Phase Status: completed

### Goal

睡眠唤醒不再等待皮肤重载；修正任务栏自动隐藏时最大化窗口被判为全屏。

### Change

- `Mascot.ini`：`DefaultUpdateDivider=-1`，三个 measure 每帧更新；`Update` 固定为 15；移除 `MascotSleepFrameMs`。
- `mascot_view.lua`：`!SetOption` 之后 `!UpdateMeter`；入睡 4 s 后 Update 只检查是否该醒；移除重载与 `MascotSleep`。
- `ui_state.inc`：移除 `MascotSleep`。
- `desktop_state.cpp`：带标题栏的前台窗口不算全屏。

### Observation

- 重载方案：从决定醒来到新皮肤第一帧约 1.12 s（日志时间戳），另加最多 250 ms 输入检测间隔。
- 本机任务栏自动隐藏，最大化的 Edge 窗口矩形 (-7,-7)–(1714,1074) 覆盖整个显示器，旧判断报告全屏，她在使用中也会入睡；修正后不再误判。
- CPU 与重绘见 `docs/measurements.md`：睡着 4.3%（关闭吉祥物 3.3%），醒着 11.8%；唤醒 2 ms。
- 截图：摸头、吃饭连续帧正常；ToDo 显示与隐藏时眼珠位置不同。

### Result

以上观测均符合预期。

## Phase 20

Phase Status: completed

### Goal

拖动时有明显的物理效果；点击区域不显示提示文字。

### Change

- `mascot_view.lua`：拖动冲量加大（`DRAG` 表）；拖动中弹簧目标按窗口速度向后偏移（`TRAIL` 表）；长发显示值平滑饱和；头部转动对长发的推力 45 → 30。拖动常量放在表里：Lua 5.1 中一个函数最多 60 个 upvalue。
- `Mascot.ini`：去掉 `HitBody` / `HitHead` 的 `ToolTipText`。

### Observation

- 逐帧日志（`docs/measurements.md`）：拖动中 `#CURRENTCONFIGX#` 逐帧更新，原效果弱是系数过小（约 350 px/s 时头偏不到 1°）。调整后约 1000 px/s 时头约 5°、尾巴约 −15°，松手后头发和尾巴回摆约 2 s。
- 截图：拖动中尾巴后飘、头倾斜；松手后长发与尾巴左右甩动。

### Result

以上观测均符合预期。

## Phase 21

Phase Status: completed

### Goal

输入框与皮肤当前风格一致；打开 ToDo 窗口时吉祥物写笔记。

### Change

- `src/prompt/prompt_theme.{h,cpp}`（新）：由皮肤颜色字符串得到输入窗口颜色。
- `prompt_window.{h,cpp}`：使用 `PromptTheme`；去掉 `WS_BORDER`，改用 DWM 圆角与 `ColorAccent` 描边；`todo` 链接 `dwmapi`。
- `command.h`、`command_parser.cpp`、`command_runner.cpp`：`prompt` 新增必需参数 `--color-bg/--color-panel/--color-text/--color-dim/--color-accent`。
- `task_view.lua`、`chat_view.lua`：打开输入框时传入当前风格颜色。
- `mascot_view.lua`：ToDo 窗口由隐藏变为显示时开始 `write` 动作。

### Observation

- 截图（DPI 感知的屏幕截图）：风格 A、B、C 下输入窗口分别为深灰底蓝描边、米白底白输入区深蓝描边、近黑底浅蓝描边，均为圆角。
- 隐藏再显示 ToDo 后 0.8 s、2.8 s 吉祥物处于写字姿势，5.8 s 时已结束。

### Result

以上观测均符合预期。

## Phase 22

Phase Status: completed

### Goal

双击吉祥物打开 ToDo 时写笔记，而不是吃饭。

### Change

- `Mascot.ini`：点击区域调用 `ClickHead()` / `ClickBody()`。
- `mascot_view.lua`：单击等待 0.45 s 再摸头 / 喂饭；`ToggleTodo()` 取消等待中的单击并忽略随后 0.45 s 内的单击。

### Observation

- 原因：双击的第一下松开时执行了单击动作 `Eat()`，ToDo 显示时她已在吃饭，写笔记不打断正在进行的动作。
- 真实鼠标测试（`mouse_event`）：双击隐藏 ToDo 后未吃饭；再次双击打开后 0.9 s、1.9 s 处于写字姿势；单击 1.5 s 后在吃饭。

### Result

以上观测均符合预期。

## Phase 23

Phase Status: completed

### Goal

ToDo 窗口打开期间吉祥物一直写字，其它动作仍可插入，眨眼与晃动照常。

### Change

- `mascot_view.lua`：ToDo 窗口可见且醒着时姿势为 `Write`；写字中的上半身与头部目标加入一半的重心 / 歪头事件；新增抬头事件（每 6–14 s，持续 1.2–2.5 s，不显示 `EyesDown`、笔停下）；随机动作在 ToDo 可见时不抽 `write`；去掉打开时的一次性 `write` 动作。

### Observation

- 截图（约每 0.6 s 一帧）：写字姿势中出现抬头帧与眨眼帧；摸头时保持本子并显示笑脸；吃饭期间换成碗勺，结束后回到写字；`SLEEP_AFTER = 0` 时入睡并放下本子。
- CPU（单核）：醒着写字 7.3% / 7.2%，睡着 4.3%。

### Result

以上观测均符合预期。

## Phase 24

Phase Status: completed

### Goal

桌宠旁边可开关的对话气泡，用于轻量聊天和搜索（本地内容与网页）。可以读取任务、计划、笔记等全部内容，但不能写入：不能改任务、写文件或提出计划。

### Structure

- `todo.exe companion show | send <message> | new`：桌宠聊天的唯一入口。会话保存在 `<data_dir>/companion/`（`ChatStore` 以目录名区分实例），不出现在 Main 的对话列表中；`send` 与 `new` 提交内容仓库。
- `CompanionService`（`src/companion/`）：选择当前会话（今天最新的一个，没有则新建），固定的人设提示词与只读工具清单 `list_tasks, list_dir, read_file, search_files, web_search`，最近 12 轮作为上下文；对话循环复用 `ChatService`。
- 权限：`run_companion` 构建的 `ToolRegistry` 只注册上述五个只读工具，`ChatService` 再按同一清单拒绝其它调用；写工具不在该进程中注册。技能文件不参与。
- `SearchFilesTool`（`src/tools/search_tools`）：在 `PathPolicy` 允许读取的目录下按子串搜索 `.md` / `.txt`，跳过 `.git`，最多 40 条 `path:line: text`。
- `WebSearchTool` + `WebSearchClient`（`src/ai/web_search_client`）：`POST https://<api_host>/v1/tools/search {query, count}`，使用已有的 DeepSeek API key；返回标题、网址、日期、摘要。
- 皮肤 `ToDo\Companion`（`Companion.ini` + `companion_view.lua`）：对话气泡，配色取当前风格；打开时由 `DesktopState` 找到桌宠窗口并停靠在其旁边；输入用 `todo.exe prompt`；收到回答时调用桌宠 `Pat()`。桌宠右键菜单显示 / 隐藏它（`!ToggleConfig`）。

### Tasks

1. `ChatStore` 目录参数；`ChatService` 按 `ChatProfile`（提示词、工具、上下文轮数、是否附任务）发送。验证：现有 `chat` 命令不变（smoke test）。
2. `WebSearchClient`、`WebSearchTool`、`SearchFilesTool`。验证：`companion send` 触发两种搜索，TOOL 记录可见。
3. `CompanionService` 与 `companion` 命令。验证：要求它加任务 / 改计划时，没有任何写入，`git log` 只有对话记录提交。
4. `ToDo\Companion` 皮肤与桌宠菜单。验证：截图三种风格；桌宠收到回答时笑。

### Observation

- `/v1/tools/search` 用现有 DeepSeek key 返回 200，`results[]` 含 `title`、`url`、`site_name`、`date`、`summary`。
- smoke test `companion` 场景（`--today 2026-10-19`）：要求调用 `set_task_status`、`add_task` 时回答说明没有写入工具；本地搜索 `search_files` 2 条、网页搜索 `web_search` 返回结果；7 次提交中 companion 的提交只改动 `companion/` 下的文件，任务文件只有场景准备时的 `add` / `note` 提交。一天的消息进入同一个会话，`new` 另起会话。
- 真实皮肤：桌宠菜单打开气泡后停靠在桌宠左侧（物理坐标右边缘 3135，桌宠左边缘 3140）；经输入框逐字输入中文并回车，存档为正确的 UTF-8，回答显示于气泡并触发笑脸；再次切换后气泡关闭。风格 B 下截图配色与 Main 一致。

### Result

以上观测均符合预期。会话日期按 id 判断：最初按创建时间判断时，`--today` 与系统日期不同会导致每条消息另起会话。

## Phase 25

Phase Status: completed

### Goal

点击尾巴或耳鳍打开 / 关闭对话气泡。

### Change

- `export_skin_assets.py`：`layout.inc` 新增尾巴与左右耳鳍的点击椭圆（静止帧轮廓外扩 3 px）。重新导出后只有 `layout.inc` 变化，帧图片不变。
- `Mascot.ini`：`HitTail`、`HitFinL`、`HitFinR`，位于 `HitBody` / `HitHead` 之下；`MeasureCompanionWindow` 不再定时更新。
- `mascot_view.lua`：`ClickTail()` / `ClickFin(side)` 甩动该部件并切换气泡，0.45 s 内重复点击只甩动；`ToggleCompanion()` 切换前更新 `MeasureCompanionWindow`。

### Observation

- 真实鼠标点击（`mouse_event`，三轮共 12 次）：尾巴、右鳍、左鳍单击各切换一次；双击尾巴只切换一次。
- 最初按约 0.5 s 一次的定时更新读取气泡状态时，出现过一次右鳍点击未关闭气泡；改为切换前立即更新后 12 次均正确。
- 尾巴椭圆与身体矩形在裙摆右下有重叠，点击区域放在身体之下，重叠处保持喂饭。

### Result

以上观测均符合预期。

## Phase 26

Phase Status: completed

### Goal

所有时间统一为系统时区（本机为美东）。

### Change

- `util/date.{h,cpp}`：`DateTime::local_zone_label()`（Windows 时区 id、是否夏令时、UTC 偏移）与 `DateTime::local_date_of()`（带偏移的 ISO 时间 → 本地日期）。
- `chat_service.cpp`、`planner.cpp`：提示词中的当前时间注明时区，并说明用户给出的时间属于该时区。
- `search_tools.cpp`：网页搜索结果日期换算为本地日期。
- `PROJECT.md`：记录系统时区与"时间只有一个来源"的约束。

### Observation

- 已有的时间来源全部是系统本地时间：`GetLocalTime`（`Date::today_local`、`DateTime::now_local_minutes`）、皮肤的 `os.date`、git。系统时区 `Eastern Standard Time`，当前 UTC−04:00。
- 临时程序调用：`local_zone_label()` = `Eastern Standard Time, daylight saving time, UTC-04:00`；`2024-12-26T02:00:00+00:00` → 2024-12-25，`2026-07-01T03:30:00Z` → 2026-06-30，`2026-10-22T09:00:00+08:00` → 2026-10-21，`2026-10-21T23:00:00-05:00` → 2026-10-22；无偏移或非法文本返回空。
- 沙盒中问桌宠"现在几点、什么时区、北京几点"：回答 12:12、美东 EDT UTC−4、北京 10 月 23 日 00:12，均正确。

### Result

以上观测均符合预期。

## Phase 27

Phase Status: completed

### Goal

桌宠对话的人设改为中文，并参考"蓝色大肥鱼"的网络形象补充性格。

### Change

- `companion_service.cpp`：人设提示词改为中文。身份为"大肥鱼"蓝发女仆鲸鱼娘；四条性格（嘴馋爱白米饭、爱摸鱼但靠谱、小得意会自嘲、关心用户）；说话规则（简短口语、纯文本、口癖与动作限频、先答题再卖萌）；工具用法（先查再答，"查一下 / 搜一下"与会变的信息必须 `web_search`）；只读说明；三段通用示范对话。

### Observation

- 依据：网络报道中的"蓝色大肥鱼"二创形象（蓝发女仆鲸鱼娘、爱吃白饭、摸鱼、"我去吃饭了"）；人设写法建议（性格 3–5 条、示范对话定语气、口癖限频、明确行为规则）。
- 沙盒对话：自我介绍为大肥鱼；任务查询调用 `list_tasks` 并提醒顺延；要求改计划时说明只读并给出可在 ToDo 窗口说的话；喜欢吃什么答白米饭；时间计算正确；回答均为纯文本、无 emoji。
- 第一版中"查一下 Flow Matching 和 DDPM 的区别"未调用 `web_search`；补充"查一下 / 搜一下必须先搜索"后，该问题与"搜一下 DeepSeek 最近的新模型"均先调用 `web_search` 并附来源。

### Result

以上观测均符合预期。

## Phase 28

Phase Status: completed

### Goal

对话气泡只有一个输入框，贴在气泡下方随之移动；内容区可滚动并有滚动条。

### Change

- `prompt_window.{h,cpp}`、`command.h`、`command_parser.cpp`、`command_runner.cpp`：`prompt --dock-below`，停靠在锚点皮肤正下方、同宽、定时跟随、不可拖动，锚点消失时以取消关闭。
- `Companion.ini` / `companion_view.lua`：去掉气泡内的假输入栏；新增标题栏 `ask` 与滚动条 `ScrollThumb`；输入框在空闲时自动打开，发送后关闭并显示 "Thinking..."，回答后重新打开；空行回车保持输入框。

### Observation

- 真实皮肤（DPI 感知截图与窗口矩形，物理像素）：打开后输入框位于气泡下方 5 px、同宽（气泡 2965,283–3340,743，输入框 2965,748–3340,833）；发送后输入框消失、气泡显示 "Thinking..." 与 ask；回答后输入框重新出现并随气泡变矮上移；`!Move` 把气泡移到另一显示器后输入框跟随（300,300–750,656 / 300,661–750,746）；从桌宠关闭气泡后输入框随之关闭。
- 长对话：滚动条出现在内容区右侧，向上滚动时从底部移到顶部，最早一轮可见。
- 测试会话 `c20261022-3` 已从内容仓库删除。

### Result

以上观测均符合预期。

## Phase 29

Phase Status: completed

### Goal

Main.ini 由仓库中的生成脚本产生，重复的行只定义一次。

### Change

- `scripts/gen_main_ini.py`（新）：静态部分原样保存，五组重复行（任务 10、时间线 12、改动 8、未计划 6、对话列表 8）各一个模板；行数常量同时写入脚本 measure 的选项（`RowCount`、`BlockRows`、`ChangeRows`、`UnplannedRows`、`ListRows`）。`--check` 只比较不写入。
- `Main.ini`：重新生成；新增 `UnplannedRows=6`（此前由 `plan_view.lua` 的默认值提供）。

### Observation

- 模板对 44 个行块逐一还原后与原文件比较：一致。补充行数选项前，生成结果与原 Main.ini 逐字节相同；补充后差异只有 `UnplannedRows=6` 一行。
- 安装后 Rainmeter 日志中 ToDo 皮肤没有错误。

### Result

以上观测均符合预期。

## Phase 30

Phase Status: completed

### Goal

四个视图脚本共用一个协议模块（解析 todo.exe 输出、拼参数、运行命令、输入框配色），不再各自复制。

### Structure

- `@Resources/Scripts/todo_protocol.lua`（新）：返回一个表，函数显式列出：`parse_records(output)`、`quote(arg)`、`run(measure, args)`、`prompt_colors(args)`。Rainmeter 的 Script measure 只能有一个 ScriptFile，共享代码只能在运行时 `dofile` 载入；模块文件顶部写明它的接口。
- `task_view.lua`、`plan_view.lua`、`chat_view.lua`、`companion_view.lua`：删除各自的副本，改为调用模块。

### Tasks

1. 新模块与四处调用替换。验证：安装后日志无错误；任务状态切换、计划刷新、对话发送、桌宠气泡发送各执行一次，输出与行为不变。

### Observation

- 四份副本中 `parse_records`、`quote`、`run`、`prompt_colors` 完全相同；`unescape` 有三种有意的差别（任务行把换行与 TAB 变成空格，计划保留换行、TAB 为一个空格，对话保留换行、TAB 为四个空格），因此 `parse_records` 接收 `format` 参数，各视图在一行包装中写明自己的格式。四个脚本共减少 112 行。
- 安装后（只读操作）：计划页、任务页、对话列表正常显示；打开桌宠气泡后输入框出现，关闭气泡后输入框关闭；本次安装后的 Rainmeter 日志没有 ToDo 错误。

### Result

以上观测均符合预期。验证改为只读操作，没有在真实数据上切换任务状态或发送对话。

## Phase 31

Phase Status: completed

### Goal

`CommandRunner` 按命令领域拆分；对话工具注册表只在一处构建。

### Structure

- `CommandRunner` 只负责分发与公共的提交 / 参数检查。
- `TaskCommands`（list、add、status、note、place、estimate、sync）、`ResearchCommands`、`ConversationCommands`（ask、chat、skill）、`CompanionCommands`、`PlanCommands`、`PromptCommand`：各自一对 `.h/.cpp`，位于 `src/cli/`。
- `ConversationTools`（`src/cli/conversation_tools.*`）：构建对话用的 `ToolRegistry`（完整集合或只读集合）及其依赖对象。

### Tasks

1. 拆分。验证：拆分前后 `smoke_test.py` 的离线场景（tasks、places、rollover、git、research、tools、skills）输出在去掉时间戳与 commit 哈希后相同；`companion` 场景中写入工具仍不存在。

### Observation

- `command_runner.cpp` 由 606 行变为 80 行（分发与 `config`）；各处理器 60–150 行。DeepSeek 客户端的构建集中到 `CommandContext::deepseek()`，工具注册表集中到 `ConversationTools`。
- smoke test（tasks、places、rollover、git、research、tools、skills）：规整 commit 哈希、耗时、临时目录、时间戳后，拆分前后均为 565 行，差异只在两段模型回答的措辞。第一次拆分后的运行中，`tools` 场景的确认窗口测试（未重新编译的 `tools_probe`，按键发送依赖前台焦点）超时；结束残留进程后重跑通过。
- `companion` 场景：只读注册表核对通过；要求写入时没有写入工具，companion 的提交只改动 `companion/`。

### Result

以上观测均符合预期。

## Phase 32

Phase Status: completed

### Goal

安装时发现皮肤脚本错误；桌宠脚本的模块级状态归入少数几个表，给 Lua 5.1 的 60 个 upvalue 上限留出余量。

### Structure

- `install_skin.ps1`：激活后读取 Rainmeter.log 中本次安装之后的 `ToDo\` 错误行，有则列出并以非零退出。
- `mascot_view.lua`：把弹簧、窗口 / 拖动、睡眠、动作等状态合并成表；`Update()` 引用的上值数量通过脚本统计输出。

### Tasks

1. 安装检查。验证：故意放入一个语法错误时安装失败并给出行号；恢复后通过。
2. 状态整理。验证：`Update()` 的上值数量下降；截图与 CPU 与整理前一致。

### Observation

- 安装检查：在 `mascot_view.lua` 末尾加入未闭合的括号后安装，退出码 1，列出 `mascot_view.lua:762: unexpected symbol near '<eof>'` 与 `[MeasureMascot]: Script: File not valid`；恢复后退出码 0。
- 状态整理（只改名）：常量并入 `ACTIONS`、`GAZE`、`DRAG.speed_max`、`SLEEP`；状态并入 `rig`、`iris`、`blink`、`window`、`sleep_state`。按脚本统计，模块级局部变量 90 → 61，`Update()` 引用的上值 55 → 40（上限 60），`Initialize()` 48 → 31。
- 验证不使用鼠标、键盘或抢焦点的窗口：安装检查无错误；命令触发摸头、吃饭，截图正常；临时设 `SLEEP.after = 0` 后入睡并放下本子，睡着 3.6%；整理前后脚本交替测醒着 CPU 各三轮（旧 13.1 / 24.1 / 16.8%，新 12.5 / 13.3 / 29.6%），差异在波动范围内；之后的日志中没有 ToDo 错误。

### Result

以上观测均符合预期。
