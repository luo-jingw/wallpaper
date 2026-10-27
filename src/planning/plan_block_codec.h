#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "planning/plan.h"

namespace todo {

// Validates a time block and converts it between the model's JSON object and the plan file line
//   - <start>-<end> <kind> <place> <title>[ (<task_id>)]
class PlanBlockCodec {
public:
    // object: {"start":..,"end":..,"kind":..,"place":..,"title":..,"task_id":..|null}
    static PlanBlock from_json(const nlohmann::json& object, bool week);
    static std::string to_line(const PlanBlock& block);  // without the leading "- "
    static PlanBlock from_line(const std::string& line, bool week);  // without the leading "- "
    // d20261020-1, w2026W43-2, l20261020-3
    static bool is_task_id(const std::string& text);
};

}  // namespace todo
