#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "skills/skill.h"

namespace todo {

class SkillStore {
public:
    explicit SkillStore(std::filesystem::path data_dir);

    // Writes assistant.md, planner.md, replan.md, and research.md only if skills/ has no .md file.
    void ensure_defaults() const;
    std::vector<Skill> list() const;
    Skill load(const std::string& name) const;

private:
    Skill parse(const std::string& name, const std::string& utf8_text, const std::string& source_name) const;
    bool name_ok(const std::string& name) const;

    std::filesystem::path skills_dir_;
};

}  // namespace todo
