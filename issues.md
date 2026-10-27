# ISSUE-001

Status: open

Area: Planning

## Observation

`plan day` / `plan week` 返回的通勤块时长由模型决定，程序不检查。`profile/places.md` 写明 remote 与校园单程约 1 小时，但同一份输入下出现过 15 分钟（09:45-10:00）和 20 分钟（周四 11:40-12:00）的通勤块。模型也会建议与现状相同的改动（例如对已是 lab 的任务建议 `set_place ... lab`）。

## Impact

计划中的时间可能不可行；Changes 中出现无效条目。

## Evidence

`scripts/smoke_test.py plan`，2026-10-20 两次运行：通勤块分别为 2 / 75 分钟、1 / 60 分钟；周计划 6 块共 320 分钟。

皮肤验证用的沙盒数据上另外三次生成：出现 30 分钟的通勤块（10:00-10:30），把"走向实验室"等过渡标为 `break` 且 place 为 `class`，以及排到 23:30 的任务块（profile 写明 18:30 结束）。

Phase 8 之后：`propose_plan` 仍会给出与现状相同的改动（`set_place w2026W43-1 lab`，该任务已是 lab）；周计划冒烟测试中出现 20 分钟的通勤块。

## Hypotheses

- 规则只说"remote 与校园之间要加通勤块"，没有给出时长；模型从 profile 文本推断时长不稳定。
- 没有把"与现状相同的改动不要给出"写进规则。

## Next Experiment

在规则中写明通勤块时长取自 profile，并要求省略与现状相同的改动；多次运行统计通勤块时长分布与无效改动数。

## Resolution
