#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include "planning/plan.h"

namespace todo {

// Sole owner of <data_dir>/plans/<period>.md.
// # Plan <period> <!-- created=.. status=.. [applied=1,3] -->
// ## Blocks / ## Changes ("- [n] <canonical change>") / ## Rationale
class PlanStore {
public:
    explicit PlanStore(std::filesystem::path data_dir);

    void save(const Plan& plan) const;
    std::optional<Plan> load(const std::string& period) const;

private:
    std::filesystem::path path_for(const std::string& period) const;
    std::string serialize(const Plan& plan) const;
    Plan parse(const std::string& period, const std::string& utf8_text) const;

    std::filesystem::path plans_dir_;
};

}  // namespace todo
