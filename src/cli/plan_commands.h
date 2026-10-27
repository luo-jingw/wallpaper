#pragma once

#include <string>

#include "cli/command.h"
#include "cli/command_context.h"
#include "domain/task_level.h"
#include "planning/plan.h"

namespace todo {

// plan day | week | show <day|week> | apply <period> <all|n,n,...> | discard <period>.
class PlanCommands {
public:
    explicit PlanCommands(const CommandContext& context) : context_(context) {}

    void run(const Command& command) const;

private:
    void propose(TaskLevel scope, const Command& command) const;
    void apply(const Command& command) const;
    void discard(const Command& command) const;
    // The plan records followed by the TASK records of the day and week files the plan refers to.
    void report_plan(const Plan& plan) const;
    void report_period_tasks(const std::string& period) const;

    const CommandContext& context_;
};

}  // namespace todo
