#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "util/date.h"

namespace todo {

struct ResearchNote {
    std::filesystem::path path;
    std::string date;  // YYYY-MM-DD
    std::string title;
};

// Sole owner of the research notes under <data_dir>/research.
class ResearchStore {
public:
    explicit ResearchStore(std::filesystem::path data_dir);

    // Writes <research>/YYYY-MM-DD-<slug>.md; a numeric suffix (-2, -3, ...) keeps existing notes untouched.
    ResearchNote create(const std::string& title, const Date& today) const;
    // Newest first, ordered by file name descending.
    std::vector<ResearchNote> list() const;

private:
    static std::string slug_for(const std::string& title);
    ResearchNote read(const std::filesystem::path& path) const;

    std::filesystem::path research_dir_;
};

}  // namespace todo
