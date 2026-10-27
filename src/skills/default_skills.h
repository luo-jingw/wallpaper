#pragma once

#include <string>
#include <utility>
#include <vector>

namespace todo {

class DefaultSkills {
public:
    static std::vector<std::pair<std::string, std::string>> files();  // filename, utf-8 contents
};

}  // namespace todo
