// Observes PathPolicy, task tools, and file tools. File writes to allowed directories
// pop ConfirmWindow (Enter allows, Esc denies). Usage:
//   tools_probe policy  <data_dir> <allowed_dir>
//   tools_probe tasks   <data_dir>
//   tools_probe file    <data_dir> <allowed_dir> <tool_name> <json>

#include <fcntl.h>
#include <io.h>

#include <cstdio>
#include <exception>
#include <filesystem>
#include <memory>
#include <string>

#include "prompt/confirm_window.h"
#include "storage/task_file.h"
#include "storage/task_repository.h"
#include "tools/file_tools.h"
#include "tools/path_policy.h"
#include "tools/task_tools.h"
#include "tools/tool_registry.h"
#include "util/date.h"
#include "util/utf8.h"

namespace {

void print(const std::string& line) {
    const std::string text = line + "\n";
    std::fwrite(text.data(), 1, text.size(), stdout);
    std::fflush(stdout);
}

std::string kind_name(todo::AccessKind kind) {
    return kind == todo::AccessKind::Read ? "read" : "write";
}

void print_decision(const todo::PathPolicy& policy, const std::string& requested, todo::AccessKind kind) {
    const todo::AccessDecision decision = policy.check(requested, kind);
    print("POLICY\t" + requested + "\t" + kind_name(kind) + "\t" + (decision.allowed ? "yes" : "no") + "\t" +
          (decision.needs_confirmation ? "yes" : "no") + "\t" + (decision.in_content_repo ? "yes" : "no") + "\t" +
          todo::Utf8::from_path(decision.resolved) + "\t" + decision.reason);
}

void run_policy(const std::filesystem::path& data_dir, const std::filesystem::path& allowed_dir) {
    std::filesystem::create_directories(data_dir / "tasks");
    std::filesystem::create_directories(allowed_dir);
    todo::Utf8::write_file(data_dir / "tasks" / "note.md", "inside\n");
    todo::Utf8::write_file(allowed_dir / "extra.md", "allowed\n");

    const todo::PathPolicy policy(data_dir, {allowed_dir});
    print_decision(policy, "tasks/note.md", todo::AccessKind::Read);
    print_decision(policy, "tasks/note.md", todo::AccessKind::Write);
    print_decision(policy, "Tasks/note.md", todo::AccessKind::Read);
    print_decision(policy, "../outside.md", todo::AccessKind::Read);
    print_decision(policy, todo::Utf8::from_path(allowed_dir / "extra.md"), todo::AccessKind::Read);
    print_decision(policy, todo::Utf8::from_path(allowed_dir / "extra.md"), todo::AccessKind::Write);
    print_decision(policy, todo::Utf8::from_path(allowed_dir / ".." / "outside.md"), todo::AccessKind::Write);
    print_decision(policy, ".git/config", todo::AccessKind::Read);
    print_decision(policy, "", todo::AccessKind::Read);

    std::error_code error;
    const std::filesystem::path link = data_dir / "link_out";
    std::filesystem::create_directory_symlink(allowed_dir, link, error);
    if (error) {
        print("SYMLINK\tskipped\t" + error.message());
    } else {
        print_decision(policy, "link_out/extra.md", todo::AccessKind::Write);
    }
}

void print_outcome(const std::string& name, const todo::ToolOutcome& outcome) {
    print("TOOL\t" + name + "\t" + todo::ToolStatusCodec::to_name(outcome.status) + "\t" +
          (outcome.changed_content_repo ? "changed" : "unchanged") + "\t" + outcome.summary);
    print("RESULT\t" + outcome.result_text);
}

void run_tasks(const std::filesystem::path& data_dir) {
    const todo::Date today = *todo::Date::parse("2026-10-20");
    const todo::TaskRepository tasks(data_dir);
    const todo::TaskToolContext context{tasks, today};
    todo::ToolRegistry registry;
    registry.add(std::make_unique<todo::ListTasksTool>(context));
    registry.add(std::make_unique<todo::AddTaskTool>(context));
    registry.add(std::make_unique<todo::SetTaskStatusTool>(context));
    registry.add(std::make_unique<todo::AddTaskNoteTool>(context));
    registry.add(std::make_unique<todo::SetTaskPlaceTool>(context));

    const auto run = [&](const std::string& name, const std::string& json) {
        print_outcome(name, registry.execute({name, name, json}, registry.names()));
    };
    run("list_tasks", "{}");
    run("add_task", R"({"level":"day","title":"跑对比实验 B","place":"lab","estimate_minutes":120})");
    run("add_task", R"({"level":"day","title":"读论文","place":"remote"})");
    run("set_task_status", R"({"task_id":"d20261020-1","status":"doing"})");
    run("add_task_note", R"({"task_id":"d20261020-1","text":"已跑 3/5"})");
    run("set_task_place", R"({"task_id":"d20261020-2","place":"class","estimate_minutes":90})");
    run("set_task_place", R"({"task_id":"d20261020-2","estimate_minutes":0})");
    run("list_tasks", R"({"level":"day"})");
    run("add_task", R"({"level":"inbox","title":"x"})");
    run("set_task_status", R"({"task_id":"missing","status":"done"})");
    print("FILE\n" + todo::Utf8::read_file(data_dir / "tasks" / "days" / "2026-10-20.md"));
}

void run_file(const std::filesystem::path& data_dir, const std::filesystem::path& allowed_dir,
              const std::string& name, const std::string& json) {
    std::filesystem::create_directories(data_dir);
    std::filesystem::create_directories(allowed_dir);
    const todo::PathPolicy policy(data_dir, {allowed_dir});
    const todo::ConfirmWindow confirm;
    const todo::FileToolContext context{policy, confirm, L""};
    todo::ToolRegistry registry;
    registry.add(std::make_unique<todo::ListDirTool>(context));
    registry.add(std::make_unique<todo::ReadFileTool>(context));
    registry.add(std::make_unique<todo::WriteFileTool>(context));
    registry.add(std::make_unique<todo::ReplaceInFileTool>(context));
    print_outcome(name, registry.execute({name, name, json}, registry.names()));
}

}  // namespace

int wmain(int argc, wchar_t** argv) {
    _setmode(_fileno(stdout), _O_BINARY);
    if (argc < 3) {
        print("usage: tools_probe policy|tasks|file ...");
        return 2;
    }
    try {
        const std::string verb = todo::Utf8::from_wide(argv[1]);
        const std::filesystem::path data_dir = argv[2];
        if (verb == "policy") {
            if (argc != 4) {
                print("usage: tools_probe policy <data_dir> <allowed_dir>");
                return 2;
            }
            run_policy(data_dir, argv[3]);
        } else if (verb == "tasks") {
            run_tasks(data_dir);
        } else if (verb == "file") {
            if (argc != 6) {
                print("usage: tools_probe file <data_dir> <allowed_dir> <tool> <json>");
                return 2;
            }
            run_file(data_dir, argv[3], todo::Utf8::from_wide(argv[4]), todo::Utf8::from_wide(argv[5]));
        } else {
            print("unknown verb: " + verb);
            return 2;
        }
    } catch (const std::exception& error) {
        print(std::string("error: ") + error.what());
        return 1;
    }
    return 0;
}
