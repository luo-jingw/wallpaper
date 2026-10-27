#pragma once

#include <string>
#include <vector>

#include "planning/plan.h"
#include "storage/task_repository.h"
#include "util/date.h"

namespace todo {

// Applies selected changes of a proposed plan to the task files, through the add_task and set_task_place
// tools, then marks the plan applied. Never commits and never saves the plan file.
class PlanApplier {
public:
    explicit PlanApplier(const TaskRepository& tasks);

    // Every number must exist and every referenced task must be found before any file changes.
    // Returns the applied change descriptions.
    std::vector<std::string> apply(Plan& plan, const std::vector<int>& numbers, const Date& today) const;

private:
    const TaskRepository& tasks_;
};

}  // namespace todo
