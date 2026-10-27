#pragma once

#include <nlohmann/json.hpp>

#include "ai/deepseek_client.h"
#include "storage/task_repository.h"
#include "tools/tool.h"
#include "util/date.h"

namespace todo {

// Task tools read and write through TaskRepository for the files of `today`; they never commit.
struct TaskToolContext {
    const TaskRepository& tasks;
    Date today;
};

class ListTasksTool : public Tool {
public:
    explicit ListTasksTool(TaskToolContext context) : context_(context) {}
    ToolSpec spec() const override;
    ToolOutcome execute(const nlohmann::json& arguments) const override;

private:
    TaskToolContext context_;
};

class AddTaskTool : public Tool {
public:
    explicit AddTaskTool(TaskToolContext context) : context_(context) {}
    ToolSpec spec() const override;
    ToolOutcome execute(const nlohmann::json& arguments) const override;

private:
    TaskToolContext context_;
};

class SetTaskStatusTool : public Tool {
public:
    explicit SetTaskStatusTool(TaskToolContext context) : context_(context) {}
    ToolSpec spec() const override;
    ToolOutcome execute(const nlohmann::json& arguments) const override;

private:
    TaskToolContext context_;
};

class AddTaskNoteTool : public Tool {
public:
    explicit AddTaskNoteTool(TaskToolContext context) : context_(context) {}
    ToolSpec spec() const override;
    ToolOutcome execute(const nlohmann::json& arguments) const override;

private:
    TaskToolContext context_;
};

class SetTaskPlaceTool : public Tool {
public:
    explicit SetTaskPlaceTool(TaskToolContext context) : context_(context) {}
    ToolSpec spec() const override;
    ToolOutcome execute(const nlohmann::json& arguments) const override;

private:
    TaskToolContext context_;
};

}  // namespace todo
