# OPT-001

Status: open

Area: Task Storage, Desktop Skin

## Observation

临时冒出的任务需要立即确定所属层级（日 / 周 / 长期）才能记录。

## Opportunity

提供一个收件箱（Inbox），先一键记录，之后再整理到具体层级。

## Expected Mechanism

新增 `tasks/inbox.md` 和 `TaskLevel::Inbox`；`todo.exe move <task_id> <level>` 把任务移到目标层级。

## Required Evidence

使用一段时间后，统计直接添加到今日、但当天被改为“已放弃”或被移到其它层级的任务数量。

## Promotion Condition

上述数量说明临时任务的分类成本确实存在。

# OPT-002

Status: open

Area: Chat, Task Storage

## Observation

“忘了做过什么”目前只能通过翻看过去的日文件或 `git log` 回答。

## Opportunity

生成每周回顾：汇总本周完成、顺延、放弃的任务，可选让 DeepSeek 生成摘要。

## Expected Mechanism

`todo.exe review week [--ai]` 读取本周日文件，输出统计记录；`--ai` 时把统计结果作为上下文调用 `ChatService`，结果写入 `research/`。

## Required Evidence

Phase 3 和 Phase 7 完成后的实际使用记录。

## Promotion Condition

使用者需要定期回顾，并且手动翻文件的成本明显。

# OPT-003

Status: open

Area: Task Domain, Desktop Skin

## Observation

任务没有截止时间，也没有提醒。

## Opportunity

为任务增加可选的截止日期，并在皮肤中高亮临近或逾期的任务。

## Expected Mechanism

在任务元数据中增加 `due=YYYY-MM-DD`；`TASK` 记录增加一个字段；皮肤按剩余天数着色。

## Required Evidence

使用中出现因错过截止时间而遗漏的任务。

## Promotion Condition

上述情况反复出现。

# OPT-004

Status: open

Area: Version Control

## Observation

两个仓库都只保存在本地，磁盘损坏会丢失全部历史。

## Opportunity

推送到 GitHub 私有仓库作为备份。

## Expected Mechanism

为两个仓库分别配置私有远程；`sync` 命令在提交后可选执行 `git push`。

## Required Evidence

已确定私有远程仓库和对应的凭据作用域。

## Promotion Condition

使用者创建了私有远程仓库，并在 `PROJECT.md` 中记录了凭据作用域。

# OPT-005

Status: open

Area: Desktop Skin

## Observation

Chat 皮肤按纯文本显示回答，DeepSeek 回答中的 Markdown 标记（`**粗体**`、反引号、列表符号）原样出现。

## Opportunity

在显示前去掉或转换常见 Markdown 标记，提高可读性。

## Expected Mechanism

`chat_view.lua` 在 `turns_text` 中剥离 `**` 和反引号；或在系统提示中要求回答使用纯文本。

## Required Evidence

日常使用中长回答的阅读体验确实受影响。

## Promotion Condition

使用者认为原样标记影响阅读。
