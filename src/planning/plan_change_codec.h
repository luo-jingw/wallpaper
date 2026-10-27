#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "planning/plan.h"

namespace todo {

// Validates a suggested change and converts it between the model's JSON object and the canonical line.
// arguments always has the shape accepted by add_task (add_task) or set_task_place (set_place, set_estimate).
class PlanChangeCodec {
public:
    // object: {"op":"set_place","task_id":..,"place":..} | {"op":"set_estimate","task_id":..,"estimate_minutes":..}
    //       | {"op":"add_task","level":..,"title":..,"place":..,"estimate_minutes":..}
    static PlanChange from_json(int number, const nlohmann::json& object);
    static PlanChange from_line(int number, const std::string& description);
};

}  // namespace todo
