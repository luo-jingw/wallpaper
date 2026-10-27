#include "planning/plan_parser.h"

#include <nlohmann/json.hpp>

#include "planning/plan_block_codec.h"
#include "planning/plan_change_codec.h"
#include "planning/plan_period.h"
#include "util/text.h"
#include "util/todo_error.h"

namespace todo {

namespace {

constexpr size_t kSnippetCodePoints = 80;

}  // namespace

Plan PlanParser::parse(const std::string& json_text, const std::string& period, const std::string& created) const {
    const nlohmann::json root = nlohmann::json::parse(json_text, nullptr, false);
    if (root.is_discarded() || !root.is_object()) {
        throw TodoError("model reply is not a JSON object: " +
                        Text::truncate_code_points(Text::single_line(json_text), kSnippetCodePoints));
    }
    for (const char* key : {"blocks", "changes"}) {
        if (!root.contains(key) || !root[key].is_array()) {
            throw TodoError(std::string("model reply needs an array \"") + key + "\"");
        }
    }
    if (!root.contains("rationale") || !root["rationale"].is_string()) {
        throw TodoError("model reply needs a string \"rationale\"");
    }

    Plan plan;
    plan.period = period;
    plan.created = created;
    plan.status = "proposed";
    const bool week = PlanPeriod::is_week(period);
    for (const nlohmann::json& block : root["blocks"]) {
        plan.blocks.push_back(PlanBlockCodec::from_json(block, week));
    }
    int number = 0;
    for (const nlohmann::json& change : root["changes"]) {
        plan.changes.push_back(PlanChangeCodec::from_json(++number, change));
    }
    plan.rationale = Text::trim(root["rationale"].get<std::string>());
    return plan;
}

}  // namespace todo
