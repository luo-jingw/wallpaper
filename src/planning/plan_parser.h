#pragma once

#include <string>

#include "planning/plan.h"

namespace todo {

// Turns the model's json_object reply into a proposed Plan.
// Throws TodoError on invalid JSON or when any field does not match the schema; nothing is written.
class PlanParser {
public:
    Plan parse(const std::string& json_text, const std::string& period, const std::string& created) const;
};

}  // namespace todo
