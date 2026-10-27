#include "cli/plan_commands.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "ai/deepseek_client.h"
#include "ai/https_client.h"
#include "domain/task.h"
#include "planning/plan_applier.h"
#include "planning/plan_period.h"
#include "planning/plan_profile.h"
#include "planning/plan_store.h"
#include "planning/planner.h"
#include "storage/task_file.h"
#include "util/text.h"
#include "util/todo_error.h"

namespace todo {

void PlanCommands::run(const Command& command) const {
    const std::string& action = context_.require_positional(command, 0, "plan action");
    if (action == "day") {
        propose(TaskLevel::Day, command);
    } else if (action == "week") {
        propose(TaskLevel::Week, command);
    } else if (action == "show") {
        const std::string& scope = context_.require_positional(command, 1, "day|week");
        if (scope != "day" && scope != "week") {
            throw TodoError("plan show expects day or week");
        }
        const std::string period = PlanPeriod::of(scope == "day" ? TaskLevel::Day : TaskLevel::Week,
                                                  command.date.value_or(context_.today));
        const std::optional<Plan> plan = PlanStore(context_.config.data_dir()).load(period);
        if (plan) {
            report_plan(*plan);
            context_.output.ok("shown " + period);
        } else {
            report_period_tasks(period);
            context_.output.ok("no plan for " + period);
        }
    } else if (action == "apply") {
        apply(command);
    } else if (action == "discard") {
        discard(command);
    } else {
        throw TodoError("plan action must be one of day, week, show, apply, discard");
    }
}

void PlanCommands::propose(TaskLevel scope, const Command& command) const {
    context_.skills.ensure_defaults();
    const PlanProfile profile(context_.config.data_dir());
    profile.ensure_examples();
    const HttpsClient http;
    const DeepSeekClient client = context_.deepseek(http);
    PlanRequestContext request{DateTime::now_local_minutes(), {}};
    if (command.chat) {
        request.conversation = context_.chats.load(*command.chat).turns;
    }
    const Planner planner(client, context_.tasks, context_.skills, profile);
    const PlanProposal proposal = planner.propose(scope, command.date.value_or(context_.today), request);
    PlanStore(context_.config.data_dir()).save(proposal.plan);
    report_plan(proposal.plan);
    context_.output.usage(proposal);
    context_.commit("plan " + proposal.plan.period + ": proposed");
    context_.output.ok("proposed " + proposal.plan.period);
}

void PlanCommands::apply(const Command& command) const {
    const std::string& period = context_.require_positional(command, 1, "period");
    const std::string& selection = context_.require_positional(command, 2, "all|n,n,...");
    const PlanStore store(context_.config.data_dir());
    std::optional<Plan> plan = store.load(period);
    if (!plan) {
        throw TodoError("no plan for " + period);
    }
    std::vector<int> numbers;
    if (selection == "all") {
        for (const PlanChange& change : plan->changes) {
            numbers.push_back(change.number);
        }
    } else {
        std::string item;
        for (size_t i = 0; i <= selection.size(); ++i) {
            if (i == selection.size() || selection[i] == ',') {
                const std::optional<int> number = Text::parse_positive_int(item);
                if (!number) {
                    throw TodoError("selection must be all or n,n,..., got: " + selection);
                }
                numbers.push_back(*number);
                item.clear();
            } else {
                item += selection[i];
            }
        }
    }
    const std::vector<std::string> applied = PlanApplier(context_.tasks).apply(*plan, numbers, context_.today);
    store.save(*plan);
    report_plan(*plan);
    context_.commit("plan " + period + ": apply " + (applied.empty() ? std::string("none") : selection));
    context_.output.ok("applied " + std::to_string(applied.size()) + " changes");
}

void PlanCommands::discard(const Command& command) const {
    const std::string& period = context_.require_positional(command, 1, "period");
    const PlanStore store(context_.config.data_dir());
    std::optional<Plan> plan = store.load(period);
    if (!plan) {
        throw TodoError("no plan for " + period);
    }
    if (plan->status != "proposed") {
        throw TodoError("plan " + period + " is " + plan->status + ", only a proposed plan can be discarded");
    }
    plan->status = "discarded";
    store.save(*plan);
    report_plan(*plan);
    context_.commit("plan " + period + ": discarded");
    context_.output.ok("discarded " + period);
}

void PlanCommands::report_plan(const Plan& plan) const {
    const Date anchor = PlanPeriod::anchor(plan.period, context_.today);
    std::map<std::string, std::string> titles;
    for (const PlanChange& change : plan.changes) {
        if (change.arguments.contains("task_id")) {
            const std::string task_id = change.arguments["task_id"].get<std::string>();
            if (const std::optional<TaskLocation> location = context_.tasks.find(task_id, anchor)) {
                titles[task_id] = location->file.tasks[location->index].title;
            }
        }
    }
    context_.output.plan(plan, titles);
    report_period_tasks(plan.period);
}

void PlanCommands::report_period_tasks(const std::string& period) const {
    const Date anchor = PlanPeriod::anchor(period, context_.today);
    for (const TaskLevel level : {TaskLevel::Day, TaskLevel::Week}) {
        for (const Task& task : context_.tasks.load(level, anchor).tasks) {
            context_.output.task(task);
        }
    }
}

}  // namespace todo
