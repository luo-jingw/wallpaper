#include <exception>
#include <string>

#include "chat/chat_store.h"
#include "cli/command.h"
#include "cli/command_parser.h"
#include "cli/command_runner.h"
#include "cli/skin_output.h"
#include "config/app_config.h"
#include "process/process_runner.h"
#include "research/research_store.h"
#include "rollover/rollover_service.h"
#include "storage/task_repository.h"
#include "util/date.h"
#include "util/todo_error.h"
#include "vcs/git_client.h"

int wmain(int argc, wchar_t** argv) {
    todo::SkinOutput output;
    try {
        const todo::Command command = todo::CommandParser().parse(argc, argv);
        const todo::AppConfig config = todo::AppConfig::load(command.config_file);
        const todo::Date today = command.today.value_or(todo::Date::today_local());
        todo::TaskRepository tasks(config.data_dir());
        const todo::ResearchStore research(config.data_dir());
        const todo::ChatStore chats(config.data_dir(), "chats");
        const todo::ProcessRunner process_runner;
        const todo::GitClient git(process_runner, config.data_dir());

        // "config" is diagnostic and "prompt" only collects text; neither touches the content repository.
        if (command.verb != "config" && command.verb != "prompt") {
            git.ensure_repository();
            todo::RolloverService rollover(tasks, config.data_dir() / "tasks" / "rollover_state.md");
            const todo::RolloverResult result = rollover.ensure(today);
            if (result.performed || command.verb == "rollover") {
                output.rollover(result);
            }
            if (result.performed) {
                const std::string message = "rollover " + result.from.to_string() + " -> " + result.to.to_string() +
                                            " (day " + std::to_string(result.day_tasks_moved) + ", week " +
                                            std::to_string(result.week_tasks_moved) + ")";
                output.commit(git.commit_all(message), message);
            }
        }

        todo::CommandRunner runner(config, today, output, tasks, research, chats, git);
        runner.run(command);
        return 0;
    } catch (const todo::TodoError& error) {
        output.error(error.what());
        return 1;
    } catch (const std::exception& error) {
        output.error(std::string("internal error: ") + error.what());
        return 2;
    }
}
