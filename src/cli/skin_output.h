#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "chat/chat_service.h"
#include "chat/chat_session.h"
#include "domain/task.h"
#include "planning/plan.h"
#include "planning/planner.h"
#include "skills/skill.h"
#include "research/research_store.h"
#include "rollover/rollover_service.h"

namespace todo {

// Writes the tab-separated, line-oriented UTF-8 protocol read by the Rainmeter skin.
class SkinOutput {
public:
    SkinOutput();

    void config(const std::string& key, const std::string& value);
    void task(const Task& task);
    void rollover(const RolloverResult& result);
    void research_note(const ResearchNote& note);
    void chat(const ChatSummary& summary);
    void turn(const ChatTurn& turn);
    void skill(const Skill& skill);
    void tool(const ToolRecord& record);
    void usage(const ChatExchange& exchange);
    void usage(const PlanProposal& proposal);
    // PLAN, then BLOCK..., CHANGE..., RATIONALE. task_titles maps the task ids that changes refer to.
    void plan(const Plan& plan, const std::map<std::string, std::string>& task_titles);
    void input(const std::string& text);
    void cancel();
    void position(int offset_x, int offset_y);
    void commit(const std::optional<std::string>& sha, const std::string& message);
    void ok(const std::string& message);
    void error(const std::string& message);

private:
    void record(const std::string& type, const std::vector<std::string>& fields);
    static std::string escape(const std::string& field);
};

}  // namespace todo
