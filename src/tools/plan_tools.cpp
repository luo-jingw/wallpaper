#include "tools/plan_tools.h"

#include <optional>

#include "domain/task.h"
#include "planning/plan.h"
#include "planning/plan_format.h"
#include "planning/plan_parser.h"
#include "planning/plan_period.h"
#include "tools/tool_arguments.h"
#include "util/todo_error.h"

namespace todo {

ToolSpec ProposePlanTool::spec() const {
    return ToolSpec{
        "propose_plan",
        "Save a new proposed day or week plan for the user to review in the Plan tab. It replaces the stored "
        "plan of that period and does not change any task; the user applies the changes. Read the tasks and "
        "profile/places.md and profile/schedule.md first. " +
            PlanFormat::time_formats() + PlanFormat::rules(false).substr(PlanFormat::rules(false).find('\n') + 1),
        R"({"type":"object","properties":{)"
        R"("scope":{"type":"string","enum":["day","week"]},)"
        R"("date":{"type":"string","description":"YYYY-MM-DD inside the plan period; default today"},)"
        R"("blocks":{"type":"array","items":{"type":"object","properties":{)"
        R"("start":{"type":"string"},"end":{"type":"string"},)"
        R"("kind":{"type":"string","enum":["task","fixed","commute","break"]},)"
        R"("place":{"type":"string","enum":["lab","class","remote","any","transit"]},)"
        R"("title":{"type":"string"},"task_id":{"type":["string","null"]}},)"
        R"("required":["start","end","kind","place","title"]}},)"
        R"("changes":{"type":"array","items":{"type":"object","properties":{)"
        R"("op":{"type":"string","enum":["add_task","set_place","set_estimate"]},)"
        R"("task_id":{"type":"string"},"place":{"type":"string","enum":["any","lab","class","remote"]},)"
        R"("estimate_minutes":{"type":"integer","minimum":1},)"
        R"("level":{"type":"string","enum":["day","week","long"]},"title":{"type":"string"}},)"
        R"("required":["op"]}},)"
        R"("rationale":{"type":"string"}},)"
        R"("required":["scope","blocks","changes","rationale"]})",
    };
}

ToolOutcome ProposePlanTool::execute(const nlohmann::json& arguments) const {
    const std::string scope = ToolArguments::required_string(arguments, "scope");
    if (scope != "day" && scope != "week") {
        throw TodoError("scope must be day or week");
    }
    Date date = context_.today;
    if (const std::optional<std::string> text = ToolArguments::optional_string(arguments, "date")) {
        const std::optional<Date> parsed = Date::parse(*text);
        if (!parsed) {
            throw TodoError("date must be YYYY-MM-DD, got: " + *text);
        }
        date = *parsed;
    }
    const std::string period = PlanPeriod::of(scope == "day" ? TaskLevel::Day : TaskLevel::Week, date);
    nlohmann::json body = nlohmann::json::object();
    for (const char* key : {"blocks", "changes", "rationale"}) {
        if (arguments.contains(key)) {
            body[key] = arguments[key];
        }
    }
    const Plan plan = PlanParser().parse(body.dump(), period, context_.now);
    context_.plans.save(plan);
    const std::string summary = "proposed " + period + ", " + std::to_string(plan.blocks.size()) + " blocks, " +
                                std::to_string(plan.changes.size()) + " changes";
    return ToolOutcome{ToolStatus::Ok,
                       "ok: " + summary + ". The user reviews it in the Plan tab and applies the changes there.",
                       true, summary};
}

}  // namespace todo
