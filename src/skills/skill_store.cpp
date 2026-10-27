#include "skills/skill_store.h"

#include <algorithm>
#include <utility>

#include "skills/default_skills.h"
#include "util/text.h"
#include "util/todo_error.h"
#include "util/utf8.h"

namespace todo {

SkillStore::SkillStore(std::filesystem::path data_dir) : skills_dir_(std::move(data_dir) / "skills") {}

bool SkillStore::name_ok(const std::string& name) const {
    if (name.empty() || name.size() > 40) {
        return false;
    }
    for (const char c : name) {
        const bool ok = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
        if (!ok) {
            return false;
        }
    }
    return true;
}

void SkillStore::ensure_defaults() const {
    if (std::filesystem::exists(skills_dir_)) {
        for (const auto& entry : std::filesystem::directory_iterator(skills_dir_)) {
            if (entry.path().extension() == L".md") {
                return;
            }
        }
    }
    for (const auto& [filename, contents] : DefaultSkills::files()) {
        Utf8::write_file(skills_dir_ / Utf8::to_path(filename), contents);
    }
}

std::vector<Skill> SkillStore::list() const {
    std::vector<Skill> skills;
    if (!std::filesystem::exists(skills_dir_)) {
        return skills;
    }
    for (const auto& entry : std::filesystem::directory_iterator(skills_dir_)) {
        if (entry.path().extension() != L".md") {
            continue;
        }
        const std::string name = Utf8::from_path(entry.path().stem());
        if (!name_ok(name)) {
            continue;
        }
        skills.push_back(parse(name, Utf8::read_file(entry.path()), Utf8::from_path(entry.path().filename())));
    }
    std::sort(skills.begin(), skills.end(), [](const Skill& a, const Skill& b) { return a.name < b.name; });
    return skills;
}

Skill SkillStore::load(const std::string& name) const {
    if (!name_ok(name)) {
        throw TodoError("invalid skill name: " + name);
    }
    const std::filesystem::path path = skills_dir_ / Utf8::to_path(name + ".md");
    if (!std::filesystem::exists(path)) {
        throw TodoError("skill not found: " + name);
    }
    return parse(name, Utf8::read_file(path), Utf8::from_path(path.filename()));
}

Skill SkillStore::parse(const std::string& name, const std::string& utf8_text, const std::string& source_name) const {
    const std::vector<std::string> lines = Text::split_lines(utf8_text);
    size_t i = 0;
    while (i < lines.size() && Text::trim(lines[i]).empty()) {
        ++i;
    }
    if (i >= lines.size() || !Text::starts_with(lines[i], "# ")) {
        throw TodoError(source_name + ": skill file must start with '# <title>'");
    }
    Skill skill;
    skill.name = name;
    skill.title = Text::trim(lines[i].substr(2));
    ++i;
    while (i < lines.size() && Text::trim(lines[i]).empty()) {
        ++i;
    }
    if (i < lines.size() && Text::starts_with(Text::trim(lines[i]), "<!--") &&
        lines[i].find("tools=") != std::string::npos) {
        const std::string line = Text::trim(lines[i]);
        const auto key = line.find("tools=");
        const auto close = line.find("-->", key);
        if (close == std::string::npos) {
            throw TodoError(source_name + ": unterminated tools comment");
        }
        std::string list = line.substr(key + 6, close - key - 6);
        if (!list.empty() && list.back() == ' ') {
            list.pop_back();
        }
        std::string item;
        for (const char c : list) {
            if (c == ',') {
                item = Text::trim(item);
                if (!item.empty()) {
                    skill.tools.push_back(item);
                }
                item.clear();
            } else {
                item += c;
            }
        }
        item = Text::trim(item);
        if (!item.empty()) {
            skill.tools.push_back(item);
        }
        ++i;
    }
    while (i < lines.size() && Text::trim(lines[i]).empty()) {
        ++i;
    }
    std::string prompt;
    for (; i < lines.size(); ++i) {
        if (!prompt.empty()) {
            prompt += '\n';
        }
        prompt += lines[i];
    }
    skill.system_prompt = Text::trim(prompt);
    if (skill.title.empty() || skill.system_prompt.empty()) {
        throw TodoError(source_name + ": skill needs a title and a system prompt");
    }
    return skill;
}

}  // namespace todo
