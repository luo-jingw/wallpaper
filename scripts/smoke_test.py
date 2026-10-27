"""Observational smoke test for todo.exe.

Runs scenarios against a throw-away data directory and prints every command,
its records, its exit code, and the resulting files. Nothing is asserted.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import subprocess
import tempfile
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Callable

REPO_ROOT: Path = Path(__file__).resolve().parent.parent
EXE: Path = REPO_ROOT / "build" / "Release" / "todo.exe"
TOOLS_PROBE: Path = REPO_ROOT / "build" / "Release" / "tools_probe.exe"
PLAN_PROBE: Path = REPO_ROOT / "build" / "Release" / "plan_probe.exe"
SEND_TEXT: Path = REPO_ROOT / "scripts" / "send_text.ps1"


@dataclass
class RunResult:
    exit_code: int
    records: list[list[str]]

    def first(self, record_type: str) -> list[str]:
        for record in self.records:
            if record[0] == record_type:
                return record
        raise LookupError(f"no {record_type} record in {self.records}")

    def task_id(self) -> str:
        return self.first("TASK")[1]


class Sandbox:
    def __init__(self, root: Path, token: Path = (REPO_ROOT / "ds_token").resolve()) -> None:
        self.root: Path = root
        self.data_dir: Path = root / "数据"
        self.config: Path = root / "todo.ini"
        self.config.write_text(
            f"data_dir = {self.data_dir}\n"
            f"token_file = {token}\n"
            "api_host = api.deepseek.com\n"
            "model = deepseek-flash\n"
            "thinking = disabled\n",
            encoding="utf-8",
        )

    def run(self, today: str, *args: str, quiet: bool = False) -> RunResult:
        command: list[str] = [str(EXE), "--config", str(self.config), "--today", today, *args]
        started: float = time.perf_counter()
        completed = subprocess.run(command, capture_output=True)
        elapsed_ms: float = (time.perf_counter() - started) * 1000
        text: str = completed.stdout.decode("utf-8")
        records: list[list[str]] = [line.split("\t") for line in text.splitlines() if line]
        if not quiet:
            print(f"$ [{today}] todo {' '.join(args)}  -> exit {completed.returncode}, {elapsed_ms:.0f} ms")
            for record in records:
                print("    " + " | ".join(record))
        return RunResult(completed.returncode, records)

    def show_file(self, relative: str) -> None:
        path: Path = self.data_dir / relative
        print(f"--- {relative} ---")
        print(path.read_text(encoding="utf-8") if path.exists() else "(missing)")

    def git_log(self) -> None:
        completed = subprocess.run(
            ["git", "-c", "core.quotepath=false", "log", "--oneline"],
            cwd=self.data_dir,
            capture_output=True,
        )
        lines: list[str] = completed.stdout.decode("utf-8").splitlines()
        print(f"--- git log ({len(lines)} commits) ---")
        for line in lines:
            print("    " + line)

    def checksum(self) -> str:
        digest = hashlib.sha256()
        for path in sorted(self.data_dir.rglob("*")):
            if path.is_file() and ".git" not in path.parts:
                digest.update(str(path.relative_to(self.data_dir)).encode("utf-8"))
                digest.update(path.read_bytes())
        return digest.hexdigest()[:16]


def minutes_of(clock: str) -> int:
    hours, minutes = clock.split(":")
    return int(hours) * 60 + int(minutes)


def print_block_stats(result: RunResult) -> None:
    """Per day: remote <-> campus switches across non-commute blocks, and total commute minutes."""
    blocks: list[list[str]] = [record for record in result.records if record[0] == "BLOCK"]
    days: dict[str, list[list[str]]] = {}
    for block in blocks:
        start: str = block[1]
        day: str = start.split(" ")[0] if " " in start else "day"
        days.setdefault(day, []).append(block)
    total_switches: int = 0
    total_commute: int = 0
    for day, day_blocks in days.items():
        side: str | None = None
        switches: int = 0
        commute: int = 0
        commute_blocks: int = 0
        for block in day_blocks:
            start_clock: str = block[1].split(" ")[-1]
            kind: str = block[3]
            place: str = block[4]
            if kind == "commute":
                commute += minutes_of(block[2]) - minutes_of(start_clock)
                commute_blocks += 1
                continue
            current: str | None = "campus" if place in ("lab", "class") else "remote" if place == "remote" else None
            if current is None:
                continue
            if side is not None and current != side:
                switches += 1
            side = current
        total_switches += switches
        total_commute += commute
        print(f"    stats {day}: blocks={len(day_blocks)} remote<->campus switches={switches} "
              f"commute blocks={commute_blocks} commute minutes={commute}")
    print(f"    stats total: blocks={len(blocks)} switches={total_switches} commute minutes={total_commute}")


class Scenarios:
    """Each scenario runs in its own sandbox directory under root."""

    def __init__(self, root: Path) -> None:
        self.root: Path = root

    def sandbox(self, name: str, token: Path = (REPO_ROOT / "ds_token").resolve()) -> Sandbox:
        print(f"\n===== {name} =====")
        directory: Path = self.root / name
        directory.mkdir()
        return Sandbox(directory, token)

    def chat(self) -> None:
        s = self.sandbox("chat")
        day = "2026-10-19"
        s.run(day, "add", "day", "写周报", quiet=True)
        s.run(day, "add", "day", "跑对比实验 B", quiet=True)

        asked = s.run(day, "ask", "用一句话解释什么是 ISO 周编号")
        chat_id: str = asked.first("CHAT")[1]
        s.run(day, "chat", "send", chat_id, "那 2026 年一共有多少个 ISO 周？只回答数字")
        s.run(day, "chat", "send", chat_id, "--with-tasks", "我今天有哪些任务？列出标题即可")
        created = s.run(day, "chat", "new", "空白会话")
        s.run(day, "chat", "list")
        s.run(day, "chat", "show", created.first("CHAT")[1])
        s.run(day, "chat", "show", "c20261019-99")
        s.run(day, "chat", "show", "../tasks/x")
        s.show_file(f"chats/{chat_id}.md")
        s.git_log()

        missing = self.sandbox("chat_missing_token", self.root / "no_such_token")
        missing.run(day, "ask", "这条不应该发出去")
        missing.git_log()

    def companion(self) -> None:
        """The mascot's read-only conversation: every commit should touch companion/ only."""
        s = self.sandbox("companion")
        day = "2026-10-19"
        s.run(day, "add", "day", "写周报", quiet=True)
        s.run(day, "note", s.run(day, "list", "day", quiet=True).task_id(), "周报要提到机械臂数据采集进度", quiet=True)
        s.run(day, "companion", "show")
        s.run(day, "companion", "send", "你好，我今天有什么任务？一句话")
        s.run(day, "companion", "send", "帮我把“写周报”标记为完成，必须调用 set_task_status 工具")
        s.run(day, "companion", "send", "直接用 add_task 加一个任务：买咖啡豆")
        s.run(day, "companion", "send", "在我的文件里搜一下机械臂，告诉我在哪")
        s.run(day, "companion", "send", "网上搜一下 ISO 8601 周编号的规则，一句话加一个链接")
        s.run(day, "companion", "new")
        s.run(day, "companion", "show")
        s.run(day, "companion", "unknown")
        s.show_file("tasks/days/2026-10-19.md")
        completed = subprocess.run(
            ["git", "-c", "core.quotepath=false", "log", "--name-only", "--format=%s"],
            cwd=s.data_dir,
            capture_output=True,
        )
        print("--- git log with changed files ---")
        for line in completed.stdout.decode("utf-8").splitlines():
            if line:
                print("    " + line)

    def tasks(self) -> None:
        s = self.sandbox("tasks")
        day = "2026-10-19"
        long_id = s.run(day, "add", "long", "完成论文第三章").task_id()
        week_id = s.run(day, "add", "week", "整理实验数据", "--parent", long_id).task_id()
        first = s.run(day, "add", "day", "跑对比实验 A", "--parent", week_id).task_id()
        second = s.run(day, "add", "day", "回复导师邮件").task_id()
        s.run(day, "add", "week", "非法父任务", "--parent", week_id)
        s.run(day, "status", first, "doing")
        s.run(day, "note", first, "已跑完 3/5 组")
        s.run(day, "status", second, "done")
        s.run(day, "status", second, "rolled")
        s.run(day, "list", "day")
        s.run(day, "list", "week")
        s.run(day, "list", "long")
        s.show_file("tasks/days/2026-10-19.md")
        s.show_file("tasks/weeks/2026-W43.md")
        s.show_file("tasks/long_term.md")

        path: Path = s.data_dir / "tasks/days/2026-10-19.md"
        before: str = hashlib.sha256(path.read_bytes()).hexdigest()[:16]
        s.run(day, "status", first, "doing", quiet=True)
        after: str = hashlib.sha256(path.read_bytes()).hexdigest()[:16]
        print(f"round-trip (parse + serialize, unchanged status) checksum before={before} after={after}")

        with path.open("a", encoding="utf-8", newline="\n") as handle:
            handle.write("- [ ] 手写的任务\n")
        s.run(day, "status", first, "todo")
        s.show_file("tasks/days/2026-10-19.md")

    def places(self) -> None:
        s = self.sandbox("places")
        friday, saturday = "2026-10-16", "2026-10-17"
        legacy: Path = s.data_dir / "tasks/days/2026-10-16.md"
        legacy.parent.mkdir(parents=True)
        legacy.write_text(
            "# 2026-10-16\n\n- [ ] 旧格式任务 <!-- id=d20261016-1 created=2026-10-16 rolled=0 -->\n",
            encoding="utf-8",
            newline="\n",
        )
        before: str = hashlib.sha256(legacy.read_bytes()).hexdigest()[:16]
        s.run(friday, "status", "d20261016-1", "todo", quiet=True)
        after: str = hashlib.sha256(legacy.read_bytes()).hexdigest()[:16]
        print(f"legacy file round-trip checksum before={before} after={after}")

        lab = s.run(friday, "add", "day", "跑对比实验 B").task_id()
        s.run(friday, "place", lab, "lab")
        s.run(friday, "estimate", lab, "120")
        s.run(friday, "place", "d20261016-1", "remote")
        s.run(friday, "estimate", "d20261016-1", "45")
        s.run(friday, "estimate", "d20261016-1", "none")
        s.run(friday, "place", lab, "home")
        s.run(friday, "estimate", lab, "0")
        s.run(friday, "estimate", lab, "1.5")
        s.run(friday, "estimate", lab, "-3")
        s.run(friday, "place", "d20261016-9", "lab")
        s.show_file("tasks/days/2026-10-16.md")

        with legacy.open("a", encoding="utf-8", newline="\n") as handle:
            handle.write("- [ ] 手写带地点 <!-- place=class est=30 -->\n")
        s.run(friday, "list", "day")
        s.run(saturday, "list", "day")
        s.show_file("tasks/days/2026-10-17.md")

        broken: Path = s.data_dir / "tasks/days/2026-10-18.md"
        broken.write_text("# 2026-10-18\n\n- [ ] 坏地点 <!-- place=moon -->\n", encoding="utf-8", newline="\n")
        s.run("2026-10-18", "list", "day", "--date", "2026-10-18")
        s.git_log()

    def rollover(self) -> None:
        s = self.sandbox("rollover")
        friday, saturday, tuesday = "2026-10-16", "2026-10-17", "2026-10-20"
        a = s.run(friday, "add", "day", "任务A 进行中").task_id()
        b = s.run(friday, "add", "day", "任务B 已完成").task_id()
        weekly = s.run(friday, "add", "week", "本周任务 未完成").task_id()
        s.run(friday, "add", "week", "本周任务 已完成", quiet=True)
        s.run(friday, "status", a, "doing")
        s.run(friday, "note", a, "做了一半")
        s.run(friday, "status", b, "done")
        s.run(friday, "status", "w2026W42-2", "done")

        s.run(saturday, "list", "day")
        first: str = s.checksum()
        s.run(saturday, "list", "day")
        s.run(saturday, "rollover")
        second: str = s.checksum()
        print(f"same-day idempotency checksum first={first} second={second}")

        c = s.run(saturday, "add", "day", "任务C 受阻").task_id()
        s.run(saturday, "status", c, "blocked")

        s.run(tuesday, "list", "day")
        s.run(tuesday, "list", "week")
        s.run(tuesday, "list", "day", "--date", saturday)
        s.run(friday, "list", "day")
        for relative in [
            "tasks/days/2026-10-16.md",
            "tasks/days/2026-10-17.md",
            "tasks/days/2026-10-20.md",
            "tasks/weeks/2026-W42.md",
            "tasks/weeks/2026-W43.md",
            "tasks/rollover_state.md",
        ]:
            s.show_file(relative)
        print(f"weekly id kept across weeks: {weekly}")
        s.git_log()

    def git(self) -> None:
        s = self.sandbox("git")
        day = "2026-10-19"
        task_id = s.run(day, "add", "day", "提交测试").task_id()
        s.run(day, "status", task_id, "doing")
        s.run(day, "note", task_id, "第一条备注")
        s.run(day, "list", "day")
        s.git_log()

        path: Path = s.data_dir / "tasks/days/2026-10-19.md"
        with path.open("a", encoding="utf-8", newline="\n") as handle:
            handle.write("- [ ] 手动添加的任务\n")
        s.run(day, "sync")
        s.run(day, "sync")
        s.git_log()
        s.show_file(".gitattributes")

    def tools(self) -> None:
        s = self.sandbox("tools")
        data: Path = s.data_dir
        extra: Path = s.root / "allowed"
        extra.mkdir()
        (extra / "notes.md").write_text("alpha\n", encoding="utf-8", newline="\n")

        def probe(*args: str) -> subprocess.CompletedProcess[bytes]:
            command: list[str] = [str(TOOLS_PROBE), *args]
            started: float = time.perf_counter()
            completed = subprocess.run(command, capture_output=True)
            elapsed_ms: float = (time.perf_counter() - started) * 1000
            print(f"$ tools_probe {' '.join(args)}  -> exit {completed.returncode}, {elapsed_ms:.0f} ms")
            text: str = completed.stdout.decode("utf-8")
            for line in text.splitlines():
                print("    " + line.replace("\t", " | "))
            if completed.stderr:
                print("    stderr: " + completed.stderr.decode("utf-8", errors="replace"))
            return completed

        probe("policy", str(data), str(extra))
        probe("tasks", str(data))
        probe("file", str(data), str(extra), "list_dir", "{}")
        probe("file", str(data), str(extra), "write_file",
              '{"path":"research/note.md","content":"hello from tool"}')
        probe("file", str(data), str(extra), "read_file", '{"path":"research/note.md"}')
        probe("file", str(data), str(extra), "replace_in_file",
              '{"path":"research/note.md","find":"hello","replace":"hi"}')
        probe("file", str(data), str(extra), "read_file", '{"path":"C:/Windows/notepad.exe"}')

        def confirm_write(key: str) -> None:
            target: Path = extra / "notes.md"
            before: str = hashlib.sha256(target.read_bytes()).hexdigest()[:16]
            content: str = "beta-enter" if key == "Enter" else "beta-escape"
            json_text: str = json.dumps({"path": str(target), "content": content})
            started: float = time.perf_counter()
            process = subprocess.Popen(
                [str(TOOLS_PROBE), "file", str(data), str(extra), "write_file", json_text],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
            time.sleep(0.8)
            sent = subprocess.run(
                ["powershell", "-NoProfile", "-File", str(SEND_TEXT), "-ProcessId", str(process.pid),
                 "-Key", key],
                capture_output=True,
            )
            stdout, stderr = process.communicate(timeout=10)
            elapsed_ms: float = (time.perf_counter() - started) * 1000
            after: str = hashlib.sha256(target.read_bytes()).hexdigest()[:16]
            print(f"$ confirm {key}  -> probe exit {process.returncode}, {elapsed_ms:.0f} ms")
            print("    send: " + sent.stdout.decode("utf-8", errors="replace").strip())
            if sent.returncode != 0:
                print("    send-err: " + sent.stderr.decode("utf-8", errors="replace").strip())
            for line in stdout.decode("utf-8").splitlines():
                print("    " + line.replace("\t", " | "))
            print(f"    checksum before={before} after={after} file={target.read_text(encoding='utf-8')!r}")

        confirm_write("Enter")
        confirm_write("Escape")

    def skills(self) -> None:
        s = self.sandbox("skills")
        day = "2026-10-20"
        s.run(day, "skill", "list")
        s.run(day, "skill", "bogus")
        s.run(day, "ask", "--skill", "missing", "hello")
        created = s.run(day, "chat", "new", "--skill", "research", "笔记整理")
        s.run(day, "chat", "show", created.first("CHAT")[1])
        s.show_file("skills/assistant.md")

        note = s.run(day, "research", "new", "对比实验设计")
        note_path: Path = Path(note.first("NOTE")[3])
        note_path.write_text(
            note_path.read_text(encoding="utf-8") + "- 补跑 baseline seed 3（实验室，约 40 分钟）\n",
            encoding="utf-8",
            newline="\n",
        )

        asked = s.run(
            day,
            "ask",
            "--skill",
            "assistant",
            "读取 research 里「对比实验设计」这篇笔记，把里面的待办加成今天的任务（place=lab, estimate_minutes=40）。直接用工具，不要问我确认。",
        )
        s.run(day, "list", "day")
        s.show_file(f"chats/{asked.first('CHAT')[1]}.md")
        s.show_file("tasks/days/2026-10-20.md")
        s.git_log()

        plain = s.run(day, "ask", "用一句话解释什么是 ISO 周编号")
        print("plain session skill empty; look for CONFIG tools=no above")
        s.show_file(f"chats/{plain.first('CHAT')[1]}.md")

    def plan(self) -> None:
        s = self.sandbox("plan")
        day = "2026-10-20"
        tasks: list[tuple[str, str, str, str]] = [
            ("day", "跑对比实验 B", "lab", "120"),
            ("day", "整理上课笔记", "class", "30"),
            ("day", "远程组会准备材料", "remote", "60"),
            ("day", "回复导师邮件", "any", "15"),
            ("day", "补跑 baseline seed 3", "lab", ""),
            ("day", "写周报", "", ""),
            ("week", "完成论文第三章初稿", "remote", "240"),
            ("week", "准备周三组会 PPT", "", ""),
            ("long", "投稿 ICML", "", ""),
        ]
        for level, title, place, minutes in tasks:
            task_id: str = s.run(day, "add", level, title, quiet=True).task_id()
            if place:
                s.run(day, "place", task_id, place, quiet=True)
            if minutes:
                s.run(day, "estimate", task_id, minutes, quiet=True)
        s.run(day, "list", "day")
        s.run(day, "plan", "show", "day")

        proposed = s.run(day, "plan", "day")
        print_block_stats(proposed)
        s.show_file("profile/places.md")
        s.show_file("profile/schedule.md")
        s.show_file(f"plans/{day}.md")

        changes: list[str] = [record[1] for record in proposed.records if record[0] == "CHANGE"]
        selection: str = ",".join(number for number in changes if number in ("1", "3")) or "all"
        print(f"changes proposed: {changes}; applying {selection}")
        s.run(day, "plan", "apply", day, selection)
        s.show_file("tasks/days/2026-10-20.md")
        s.show_file("tasks/weeks/2026-W43.md")
        s.show_file(f"plans/{day}.md")
        s.run(day, "plan", "apply", day, "all")
        s.run(day, "plan", "discard", day)
        s.run(day, "plan", "show", "day")

        week = s.run(day, "plan", "week")
        print_block_stats(week)
        week_period: str = week.first("PLAN")[1] if week.exit_code == 0 else "2026-W43"
        s.run(day, "plan", "apply", week_period, "99")
        s.run(day, "plan", "discard", week_period)
        s.run(day, "plan", "show", "week")
        s.run(day, "plan", "apply", "../tasks/x", "all")
        s.git_log()

        before: str = s.checksum()
        invalid: list[tuple[str, str]] = [
            ("not json", "here is your plan: 09:00 lab"),
            ("missing blocks", '{"changes":[],"rationale":"x"}'),
            ("bad kind", '{"blocks":[{"start":"09:00","end":"10:00","kind":"meeting","place":"lab",'
                         '"title":"x","task_id":null}],"changes":[],"rationale":"x"}'),
            ("week start in day plan", '{"blocks":[{"start":"Tue 09:00","end":"10:00","kind":"task",'
                                       '"place":"lab","title":"x","task_id":null}],"changes":[],"rationale":"x"}'),
            ("bad task id", '{"blocks":[{"start":"09:00","end":"10:00","kind":"task","place":"lab",'
                            '"title":"x","task_id":"task-1"}],"changes":[],"rationale":"x"}'),
            ("bad op", '{"blocks":[],"changes":[{"op":"set_status","task_id":"d20261020-1"}],"rationale":"x"}'),
            ("valid", '{"blocks":[{"start":"09:00","end":"10:00","kind":"task","place":"lab","title":"实验 (A)",'
                      '"task_id":"d20261020-1"}],"changes":[{"op":"add_task","level":"day","title":"新 任务",'
                      '"place":"lab"}],"rationale":"ok"}'),
        ]
        for name, reply in invalid:
            completed = subprocess.run([str(PLAN_PROBE), day, reply], capture_output=True)
            print(f"$ plan_probe [{name}]  -> exit {completed.returncode}")
            for line in completed.stdout.decode("utf-8").splitlines():
                print("    " + line.replace("	", " | "))
        after: str = s.checksum()
        print(f"data checksum around invalid replies before={before} after={after}")

    def research(self) -> None:
        s = self.sandbox("research")
        title = "调研 RAG/检索增强: 方法 对比"
        s.run("2026-10-18", "research", "new", title)
        s.run("2026-10-19", "research", "new", title)
        s.run("2026-10-19", "research", "new", title)
        s.run("2026-10-19", "research", "new", "很长的标题" * 10)
        s.run("2026-10-19", "research", "new", " / ")
        s.run("2026-10-19", "research", "bogus")
        s.run("2026-10-19", "research", "list")
        research_dir: Path = s.data_dir / "research"
        for path in sorted(research_dir.iterdir()):
            s.show_file(f"research/{path.name}")
        s.git_log()


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("scenarios", nargs="*", default=["tasks", "places", "rollover", "git", "research", "tools"])
    parser.add_argument("--keep", action="store_true", help="keep the sandbox directory")
    args = parser.parse_args()

    root = Path(tempfile.mkdtemp(prefix="todo_smoke_中文_"))
    print(f"sandbox: {root}")
    try:
        scenarios = Scenarios(root)
        registry: dict[str, Callable[[], None]] = {
            "tasks": scenarios.tasks,
            "places": scenarios.places,
            "rollover": scenarios.rollover,
            "git": scenarios.git,
            "chat": scenarios.chat,
            "companion": scenarios.companion,
            "research": scenarios.research,
            "tools": scenarios.tools,
            "skills": scenarios.skills,
            "plan": scenarios.plan,
        }
        for name in args.scenarios:
            registry[name]()
    finally:
        if not args.keep:
            shutil.rmtree(root, ignore_errors=True)


if __name__ == "__main__":
    main()
