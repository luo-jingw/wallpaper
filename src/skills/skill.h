#pragma once

#include <string>
#include <vector>

namespace todo {

struct Skill {
    std::string name;
    std::string title;
    std::vector<std::string> tools;
    std::string system_prompt;
};

}  // namespace todo
