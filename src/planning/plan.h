#pragma once

#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace todo {

// One time block. Day plans use "HH:MM" for start; week plans use "Mon HH:MM". end is always "HH:MM".
struct PlanBlock {
    std::string start;
    std::string end;
    std::string kind;   // task | fixed | commute | break
    std::string place;  // lab | class | remote | any | transit
    std::string title;
    std::optional<std::string> task_id;
};

// A suggested task change. description is the canonical one-line form stored in the plan file:
//   set_place <task_id> <place>
//   set_estimate <task_id> <minutes>
//   add_task <day|week|long> <place> <minutes|-> <title>
struct PlanChange {
    int number;
    std::string op;  // add_task | set_place | set_estimate
    nlohmann::json arguments;
    std::string description;
};

struct Plan {
    std::string period;   // YYYY-MM-DD or YYYY-Www
    std::string created;  // YYYY-MM-DDTHH:MM
    std::string status;   // proposed | applied | discarded
    std::vector<PlanBlock> blocks;
    std::vector<PlanChange> changes;
    std::vector<int> applied;  // change numbers applied by plan apply
    std::string rationale;
};

}  // namespace todo
