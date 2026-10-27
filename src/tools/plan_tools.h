#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "ai/deepseek_client.h"
#include "planning/plan_store.h"
#include "tools/tool.h"
#include "util/date.h"

namespace todo {

struct PlanToolContext {
    const PlanStore& plans;
    Date today;
    std::string now;  // YYYY-MM-DDTHH:MM, recorded as the plan's created time
};

// Saves a plan written by the model as a proposed plan; it never changes tasks.
class ProposePlanTool : public Tool {
public:
    explicit ProposePlanTool(PlanToolContext context) : context_(context) {}
    ToolSpec spec() const override;
    ToolOutcome execute(const nlohmann::json& arguments) const override;

private:
    PlanToolContext context_;
};

}  // namespace todo
