#include "planning/plan_applier.h"

#include <algorithm>

#include "planning/plan_period.h"
#include "tools/task_tools.h"
#include "tools/tool.h"
#include "util/todo_error.h"

namespace todo {

PlanApplier::PlanApplier(const TaskRepository& tasks) : tasks_(tasks) {}

std::vector<std::string> PlanApplier::apply(Plan& plan, const std::vector<int>& numbers, const Date& today) const {
    if (plan.status != "proposed") {
        throw TodoError("plan " + plan.period + " is " + plan.status + ", only a proposed plan can be applied");
    }
    const Date anchor = PlanPeriod::anchor(plan.period, today);

    std::vector<int> selected = numbers;
    std::sort(selected.begin(), selected.end());
    if (std::adjacent_find(selected.begin(), selected.end()) != selected.end()) {
        throw TodoError("a change number is repeated");
    }
    std::vector<const PlanChange*> changes;
    for (const int number : selected) {
        const auto found = std::find_if(plan.changes.begin(), plan.changes.end(),
                                        [number](const PlanChange& change) { return change.number == number; });
        if (found == plan.changes.end()) {
            throw TodoError("plan " + plan.period + " has no change " + std::to_string(number));
        }
        if (found->arguments.contains("task_id")) {
            const std::string task_id = found->arguments["task_id"].get<std::string>();
            if (!tasks_.find(task_id, anchor)) {
                throw TodoError("change " + std::to_string(number) + ": task not found: " + task_id);
            }
        }
        changes.push_back(&*found);
    }

    const TaskToolContext context{tasks_, anchor};
    const AddTaskTool add_task(context);
    const SetTaskPlaceTool set_task_place(context);
    std::vector<std::string> applied;
    for (const PlanChange* change : changes) {
        const Tool& tool = change->op == "add_task" ? static_cast<const Tool&>(add_task) : set_task_place;
        tool.execute(change->arguments);
        applied.push_back(change->description);
    }
    plan.status = "applied";
    plan.applied = selected;
    return applied;
}

}  // namespace todo
