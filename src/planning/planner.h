#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "ai/deepseek_client.h"
#include "chat/chat_session.h"
#include "domain/task.h"
#include "planning/plan.h"
#include "planning/plan_profile.h"
#include "skills/skill_store.h"
#include "storage/task_repository.h"
#include "util/date.h"

namespace todo {

struct PlanProposal {
    Plan plan;
    ChatCompletion completion;
    size_t prompt_chars;  // system + user message
};

struct PlanRequestContext {
    std::string now;                     // YYYY-MM-DDTHH:MM, local
    std::vector<ChatTurn> conversation;  // recent turns of the current conversation; may be empty
};

// Asks DeepSeek (json_object) for a day or week plan built from the tasks, the profile files, the current
// time, the recent conversation, and the planner skill's system prompt.
class Planner {
public:
    Planner(const DeepSeekClient& client, const TaskRepository& tasks, const SkillStore& skills,
            const PlanProfile& profile);

    // scope: Day or Week. Throws TodoError on invalid JSON or a schema mismatch.
    PlanProposal propose(TaskLevel scope, const Date& date, const PlanRequestContext& context) const;

private:
    std::string request_text(TaskLevel scope, const Date& date, const PlanRequestContext& context) const;

    const DeepSeekClient& client_;
    const TaskRepository& tasks_;
    const SkillStore& skills_;
    const PlanProfile& profile_;
};

}  // namespace todo
