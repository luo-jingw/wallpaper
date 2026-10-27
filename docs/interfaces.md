# Interfaces

## Configuration

`todo.ini`，`key = value`，`#` 开头为注释。相对路径以 `todo.ini` 所在目录为基准。

| 键 | 值 |
|---|---|
| `data_dir` | 内容仓库目录，默认 `../ToDo-data` |
| `token_file` | API Key 文件，单行，默认 `ds_token` |
| `api_host` | `api.deepseek.com` |
| `model` | `deepseek-flash` 或 `deepseek-v4-pro` |
| `thinking` | `enabled` 或 `disabled` |
| `allowed_dirs` | 对话工具可额外读取的目录，分号分隔；可为空。这些目录内的写入会弹出确认窗口 |

## Command Line

```text
todo.exe --config <todo.ini> [--today YYYY-MM-DD] <command> [args]

  config                                    输出解析后的配置；不执行顺延与 git
  list <day|week|long> [--date YYYY-MM-DD]
  add <day|week|long> <title> [--parent ID]
  status <task_id> <todo|doing|done|blocked|dropped>
  note <task_id> <text>
  place <task_id> <any|lab|class|remote>
  estimate <task_id> <minutes|none>
  rollover
  sync                                      提交内容仓库中的手动修改
  research new <title>
  research list
  ask [--with-tasks] [--skill NAME] <question>
  chat new [--skill NAME] <title>
  chat list
  chat show <chat_id>
  chat send <chat_id> [--with-tasks] <message>
  companion show | send <message> | new     桌宠的只读对话（见下文）
  skill list
  plan <day|week> [--date YYYY-MM-DD] [--chat <chat_id>]
                                            生成并保存计划（DeepSeek json_object）；--chat 把该会话最近 12 轮放进请求
  plan show <day|week> [--date YYYY-MM-DD]  没有计划时输出 OK no plan for <period>
  plan apply <period> <all|n,n,...>         period 为 YYYY-MM-DD 或 YYYY-Www
  plan discard <period>
  prompt <caption> --anchor <skin window title> --offset-x N --offset-y N
         --color-bg C --color-panel C --color-text C --color-dim C --color-accent C [--dock-below]
                                            弹出输入窗口；不执行顺延与 git
```

`--today` 固定当天日期，省略时使用本地系统日期。

`companion`：
- 会话保存在 `<data_dir>/companion/<id>.md`（格式同 `chats/`），不出现在 `chat list` 中。当天的会话是 id 以 `c<今天>-` 开头的最新一个；`send` 在没有当天会话时新建，`new` 新建一个空会话。`send` 与 `new` 提交内容仓库，提交信息为 `companion <id>: <消息或 new>`。
- 输出 `CHAT`，然后是最近 8 轮的 `TOOL` / `TURN`；`send` 另有 `USAGE`。`show` 在当天没有会话时只输出 `OK`。
- 只读：进程内只注册 `list_tasks`、`list_dir`、`read_file`、`search_files`、`web_search`，`ChatService` 也只允许这五个工具；不读取技能文件。人设提示词（中文）在 `src/companion/companion_service.cpp`："蓝色大肥鱼"鲸鱼娘，嘴馋（白米饭）、爱摸鱼但靠谱、小得意会自嘲、关心用户；简短口语、纯文本、不用 Markdown 和 emoji，口癖与括号动作限频；涉及用户自己的事先查文件，"查一下 / 搜一下"或会变的信息先调用 `web_search`；说明自己没有写入工具并建议去 ToDo 窗口；附三段示范对话。上下文为最近 12 轮。
- `search_files(query, path?)`：在 `PathPolicy` 允许读取的目录（默认整个内容仓库）中按子串搜索 `.md` / `.txt`，拉丁字母不区分大小写，跳过 `.git` 与大于 512 KB 的文件，每个文件最多 5 条、共 40 条 `path:line: text`。
- `web_search(query, count?)`：结果日期按系统时区换算为本地日期。`POST https://<api_host>/v1/tools/search {"query","count"}`，`Authorization: Bearer <DeepSeek API key>`；返回 `results[]` 的 `title`、`url`、`site_name`、`date`、`summary`。工具结果每条摘要截到 400 字，`count` 1–8，默认 5。

`prompt`：
- Rainmeter 皮肤窗口没有开启 DPI 感知，它记录的坐标（`WindowX`、`#CURRENTCONFIGX#`）是缩放后的虚拟坐标，不能直接用于定位。`prompt` 进程为 Per-Monitor V2 DPI 感知：先按 `--anchor`（皮肤窗口标题，即 `.ini` 的完整路径）找到 `RainmeterMeterWindow`，读取它的物理矩形，再加上按该显示器 DPI 换算后的 `--offset-x/--offset-y`（皮肤单位，96 DPI）。窗口宽度等于皮肤的物理宽度；高度和字体按 DPI 缩放。找不到皮肤窗口时输出 `ERR`。
- 颜色取自皮肤当前风格（`style-{a,b,c}.inc` 的 `ColorBg`、`ColorPanel`、`ColorText`、`ColorDim`、`ColorAccent`），格式 `R,G,B` 或 `R,G,B,A`，五个参数都必须给出，格式错误时输出 `ERR`。窗口底色为 `ColorBg` 去掉透明度；输入框底色为 `ColorPanel` 按其透明度叠加在 `ColorBg` 上；说明文字用 `ColorDim`，输入文字用 `ColorText`；Windows 11 上窗口为圆角，描边为 `ColorAccent`（`DwmSetWindowAttribute`）。
- `--dock-below`：窗口贴在皮肤窗口正下方（间隔 4 个皮肤单位）、与其同宽，每 30 ms 跟随皮肤窗口的位置和宽度，不可拖动；皮肤窗口消失时以 `CANCEL` 关闭。此时 `--offset-x/--offset-y` 不起作用。
- 未停靠时，拖动窗口任意非输入区域即可移动它。退出时输出 `POSITION <offset_x> <offset_y>`（皮肤单位），皮肤把它保存为 `ui_state.inc` 中的 `TasksPromptX/Y` 或 `ChatPromptX/Y`。
- 窗口置顶，通过 `AttachThreadInput` 连接当前前台线程后取得前台和键盘焦点。
- Enter 输出 `INPUT`（文本可为空）；Esc 或关闭窗口输出 `CANCEL`。输入法组合期间的 Enter 与 Esc 交给输入法处理。

## Output Protocol

标准输出为 UTF-8，每行一条记录，字段之间用制表符分隔：

```text
CONFIG	<key>	<value>
TASK	<id>	<status>	<title>	<created>	<rolled>	<parent>	<last_note>	<place>	<estimate_minutes|>
ROLLOVER	<from>	<to>	<day_moved>	<week_moved>	<yes|no>
NOTE	<date>	<title>	<path>
CHAT	<id>	<title>	<created>	<turn_count>	<skill|>
TURN	<user|assistant>	<content>
SKILL	<name>	<title>	<tools comma-separated>
TOOL	<name>	<summary>	<ok|denied|error>
PLAN	<period>	<proposed|applied|discarded>	<created>
BLOCK	<start>	<end>	<task|fixed|commute|break>	<lab|class|remote|any|transit>	<title>	<task_id|>
CHANGE	<number>	<canonical change>	<applied|>	<title of the referenced task|>
RATIONALE	<text>
USAGE	<tokens_in>	<tokens_out>	<latency_ms>	<request_messages>	<system_chars>
INPUT	<text>
CANCEL
POSITION	<offset_x>	<offset_y>
COMMIT	<sha|none>	<message>
OK	<message>
ERR	<message>
```

字段转义：反斜杠写成 `\\`，换行写成 `\n`，制表符写成 `\t`，回车删除。失败时最后一行为 `ERR`，退出码非 0。

## Task Files

```markdown
# 2026-10-19

- [ ] 写周报 <!-- id=d20261015-2 created=2026-10-15 rolled=3 parent=w2026W42-1 -->
  > 进展：完成一半
- [x] 回邮件 <!-- id=d20261019-1 created=2026-10-19 rolled=0 -->
- [ ] 跑对比实验 B <!-- id=d20261019-2 created=2026-10-19 rolled=0 place=lab est=120 -->
```

- 标题：日文件 `YYYY-MM-DD`，周文件 `YYYY-Www`，长期文件 `Long Term`。
- 状态标记：`[ ]` todo，`[~]` doing，`[x]` done，`[!]` blocked，`[-]` dropped，`[>]` rolled。
- ID：层级前缀（`d` / `w` / `l`）+ 周期（日为 `YYYYMMDD`，周为 `YYYYWww`）+ `-序号`。
- 手动添加的 `- [ ] 标题` 行（无注释）在下一次保存时分配 ID。
- `place`（`lab` / `class` / `remote`）与 `est`（正整数分钟）可省略；`place=any` 和无时长时不写入，因此不含这两个键的旧文件保存后字节不变。顺延时两者随任务一起复制。
- `  > ` 开头的行是上一条任务的备注。无法识别的非空行使解析失败并输出 `ERR`。

## Research Notes

`research/YYYY-MM-DD-<slug>.md`，slug 取标题前若干码点，重名时追加 `-2`、`-3`：

```markdown
# 标题

<!-- created=2026-10-19 -->

```

## Chat Files

`chats/c<YYYYMMDD>-<n>.md`；ID 只允许该格式，其它输入（例如 `../tasks/x`）输出 `ERR invalid chat id`。

```markdown
# 会话标题 <!-- id=c20261019-1 created=2026-10-19T23:40 -->

## user

问题

## assistant <!-- tokens_in=120 tokens_out=480 latency_ms=3120 -->

回答
```

## Plan Files

`plans/<period>.md`；period 只允许 `YYYY-MM-DD` 或 `YYYY-Www`，其它输入输出 `ERR`。

```markdown
# Plan 2026-10-20 <!-- created=2026-10-20T01:43 status=applied applied=1,3 -->

## Blocks
- 09:00-10:00 commute transit remote -> 校园
- 10:00-11:40 fixed class 上课
- 13:00-15:00 task lab 跑对比实验 B (d20261020-1)

## Changes
- [1] set_estimate d20261020-3 75
- [2] set_place d20261020-5 lab
- [3] add_task day lab 30 整理组会记录

## Rationale
自由文本，到文件末尾。
```

周计划的时间块开始时间形如 `Tue 09:00`；结束时间为 `HH:MM`，也接受与开始同一天的 `Tue 12:00` 并规范为 `12:00`，跨天输出 `ERR`。改动行：`set_place <task_id> <any|lab|class|remote>`、`set_estimate <task_id> <minutes>`、`add_task <day|week|long> <place> <minutes|-> <title>`。task_id 格式：`d20261020-1`、`w2026W43-2`、`l20261020-3`。

模型回复（json_object）：

```json
{"blocks":[{"start":"09:00","end":"10:00","kind":"commute","place":"transit","title":"remote -> campus","task_id":null}],
 "changes":[{"op":"set_place","task_id":"d20261020-3","place":"remote"},
            {"op":"set_estimate","task_id":"d20261020-3","estimate_minutes":45},
            {"op":"add_task","level":"day","title":"...","place":"lab","estimate_minutes":30}],
 "rationale":"..."}
```

`plan show|day|week|apply|discard` 在计划记录之后输出计划周期锚定日期的日任务与周任务 `TASK` 记录；没有计划时 `plan show` 也输出它们。

对话工具 `propose_plan`（参数 `scope`、可选 `date`、`blocks`、`changes`、`rationale`，结构同上）用同一个校验器保存为 `proposed` 计划，不修改任务；默认 skill `planner`、`assistant` 可用。

## Profile Files

`profile/places.md`、`profile/schedule.md`：自由格式 Markdown，原文放入规划请求。第一次 `plan day|week` 时若不存在则写入示例（lab 与 class 相邻；remote 与校园单程约 1 小时；示例课表）。

## Content Repository Commits

每个修改类命令产生一个提交，消息为命令摘要，例如 `add <id>: <title>`、`status <id> done: <title>`、`note <id>: <text>`、`sync manual edits`、`research new: <title>`、`ask <chat_id>: <title>`、`skill defaults`、`plan <period>: proposed`、`plan <period>: apply 1,3`、`plan <period>: discarded`、`chat <chat_id>: <message 前 30 个码点>`、`rollover <from> -> <to> (day N, week M)`。没有变化时不提交，输出 `COMMIT none`。

## Install

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\install_skin.ps1 [-Config <todo.ini>] [-Activate Main,Mascot] [-Deactivate Tasks,Chat]
```

把 `rainmeter\ToDo` 复制到 Rainmeter 皮肤目录下的 `ToDo`，保留已有的 `ui_state.inc` 并补上它缺少的键，把 `build\Release\CursorPos.dll` 与 `DesktopState.dll` 复制到 `%APPDATA%\Rainmeter\Plugins`（已被 Rainmeter 加载的插件无法覆盖，需重启 Rainmeter），写入 `paths.inc`，刷新 Rainmeter，停用 `-Deactivate` 列出的皮肤并激活 `-Activate` 列出的皮肤，约 4 s 后读取本次安装以来 Rainmeter.log 中 ToDo 皮肤与其 Lua 脚本的错误行；有错误时列出并以 1 退出。复制不删除目标目录中已有的文件，旧的 `Tasks/`、`Chat/` 目录仍在安装目录中。
