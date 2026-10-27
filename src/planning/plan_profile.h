#pragma once

#include <filesystem>
#include <string>

namespace todo {

struct PlanProfileText {
    std::string places;    // profile/places.md, verbatim
    std::string schedule;  // profile/schedule.md, verbatim
};

// profile/places.md and profile/schedule.md are free-form Markdown owned by the user.
// The program never parses them; it only writes an example for a file that does not exist yet.
class PlanProfile {
public:
    explicit PlanProfile(std::filesystem::path data_dir);

    // Returns true when an example file was written.
    bool ensure_examples() const;
    PlanProfileText read() const;

private:
    std::filesystem::path places_file_;
    std::filesystem::path schedule_file_;
};

}  // namespace todo
